/***************************************************************************
 *  include/stxxl/bits/common/winner_tree.h
 *
 *  Part of the STXXL. See http://stxxl.sourceforge.net
 *
 *  Copyright (C) 2014 Thomas Keh <thomas.keh@student.kit.edu>
 *  Copyright (C) 2014 Timo Bingmann <tb@panthema.net>
 *
 *  Distributed under the Boost Software License, Version 1.0.
 *  (See accompanying file LICENSE_1_0.txt or copy at
 *  http://www.boost.org/LICENSE_1_0.txt)
 **************************************************************************/

#ifndef STXXL_COMMON_WINNER_TREE_HEADER
#define STXXL_COMMON_WINNER_TREE_HEADER

#include <vector>
#include <stack>
#include <cassert>

#include <stxxl/bits/common/timer.h>
#include <stxxl/bits/common/utils.h>
#include <stxxl/bits/verbose.h>

STXXL_BEGIN_NAMESPACE

/*!
 * The class winner_tree is a binary tournament tree. There are n=2^k so
 * called players which compete for the winning position. The winner is the
 * smallest one regarding the comparator m_less. Each player is identified with
 * it's index. The comparator should use this index in order to compare the
 * actual values.
 *
 * A change of one players value results in O(log n) operations.  The winner
 * can be determined in O(1) operations.
 *
 * There is a number of fixed players which remain at the same index forever,
 * even if they were deactivated in between. Additionally any number of players
 * can be added dynamically. They are assigned to a free slot and if one gets
 * removed the slot will be marked free again.
 */
template <typename Comparator>
class winner_tree
{
protected:
    //! the binary tree of size 2^(k+1)-1
    std::vector<int> m_tree;

    //! the comparator object of this tree
    Comparator& m_less;

    //! number of slots for the players (2^k)
    unsigned int m_num_slots;

    //! Defines if statistics are gathered: fake_timer or timer
    typedef fake_timer stats_timer;

    //! Collection of stats from the winner_tree
    struct stats_type
    {
        stats_timer replay_time;
        stats_timer double_num_slots_time;
        stats_timer remove_player_time;

        friend std::ostream& operator << (std::ostream& os, const stats_type& o)
        {
            return os << "replay_time=" << o.replay_time << std::endl
                      << "double_num_slots_time=" << o.double_num_slots_time << std::endl
                      << "remove_player_time=" << o.remove_player_time << std::endl;
        }
    };

    //! Collection of stats from the winner_tree
    stats_type m_stats;

public:
    /**
     * Constructor. Reserves space, registers free slots. No games are played
     * here! All players and slots are deactivated in the beginning.
     *
     * \param num_players Number of fixed players. Fixed players remain at the
     * same index forever, even if they were deactivated in between.
     *
     * \param less The comparator. It should use two given indices, compare the
     * corresponding values and return the index of one with the smaller value.
     */
    winner_tree(unsigned int num_players, Comparator& less)
        : m_less(less)
    {
        assert(num_players > 0);

        m_num_slots = (1 << ilog2_ceil(num_players));
        unsigned int treesize = (m_num_slots << 1) - 1;
        m_tree.resize(treesize, -1);
    }

    //! activate one of the static players or add a new one and replay
    inline void activate_player(unsigned int index)
    {
        while (index >= m_num_slots)
            double_num_slots();
        replay_on_change(index);
    }

    //! activate a player and resize if necessary
    inline void activate_without_replay(unsigned int index)
    {
        while (index >= m_num_slots)
            double_num_slots();
        m_tree[((m_tree.size() / 2) + index)] = index;
    }

    //! deactivate a player
    inline void deactivate_without_replay(unsigned int index)
    {
        assert(index < m_num_slots);
        m_tree[((m_tree.size() / 2) + index)] = -1;
    }

    //! deactivate a player and replay.
    inline void deactivate_player(unsigned int index)
    {
        assert(index < m_num_slots);
        replay_on_change(index, true);
    }

    /**
     * deactivate one player in a batch of deactivations run
     * replay_on_deactivation() afterwards. If there are multiple consecutive
     * removes you should run deactivate_player_step() for all of them first and
     * afterwards run replay_on_deactivation() for each one of them.
     */
    inline void deactivate_player_step(unsigned int index)
    {
        m_stats.remove_player_time.start();
        assert(index < m_num_slots);
        int p = (int)(m_tree.size() / 2) + index;

        // Needed for following deactivations...
        while (p >= 0 && m_tree[p] == (int)index) {
            m_tree[p] = -1;
            p -= (p + 1) % 2;
            p /= 2;
        }

        m_stats.remove_player_time.stop();
    }

    //! Replay after the player at index has been deactivated.
    inline void replay_on_deactivations(unsigned int index)
    {
        assert(index < m_num_slots);
        replay_on_change(index, true);
    }

    //! Notify that the value of the player at index has changed.
    inline void notify_change(unsigned int index)
    {
        replay_on_change(index);
    }

    //! Returns the winner.
    //! Remember running replay_on_pop() if the value of the winner changes.
    inline int top() const
    {
        return m_tree[0];
    }

    //! Returns if all players are deactivated.
    inline bool empty() const
    {
        return (m_tree[0] < 0);
    }

    //! Return the current number of slots
    size_t num_slots() const
    {
        return m_num_slots;
    }

    //! Replay after the value of the winning player has changed.
    inline void replay_on_pop()
    {
        replay_on_change(m_tree[0]);
    }

    /**
     * Replays all games the player with the given index is involved in.  This
     * corresponds to the path from leaf [index] to root.  If only the value of
     * player [index] has changed the result is a valid winner tree.
     *
     * \param index	The player whose value has changed.
     *
     * \param done	Set done to true if the player has been deactivated
     *		        or removed. All games will be lost then.
     */
    inline void replay_on_change(unsigned int index, bool done = false)
    {
        m_stats.replay_time.start();

        int top;
        int p = static_cast<int>((m_tree.size() / 2) + index);

        if (!done) {
            top = index;
        }
        else {
            top = -1;
        }

        while (p > 0) {
            m_tree[p] = top;
            p -= (p + 1) % 2;           // round down to left node position

            if (m_tree[p] < 0)
                top = m_tree[p + 1];
            else if (m_tree[p + 1] < 0)
                top = m_tree[p];
            else if (m_less(m_tree[p], m_tree[p + 1]))
                top = m_tree[p];
            else
                top = m_tree[p + 1];

            p /= 2;
        }

        m_tree[p] = top;

        m_stats.replay_time.stop();
    }

    //! Doubles the number of slots and adapts the tree so it's a valid winner
    //! tree again.
    inline void double_num_slots()
    {
        m_stats.double_num_slots_time.start();

        m_num_slots = m_num_slots << 1;
        unsigned old_tree_size = static_cast<unsigned>(m_tree.size());
        unsigned tree_size = (m_num_slots << 1) - 1;
        m_tree.resize(tree_size, -1);

        for (int i = old_tree_size - 1; i >= 0; --i) {
            unsigned old_index = i;
            unsigned old_level = ilog2_floor(old_index + 1);
            unsigned new_index = old_index + (1 << old_level);
            m_tree[new_index] = m_tree[old_index];
        }

        unsigned step_size = (1 << ilog2_floor(old_tree_size));
        unsigned index = tree_size - 1;

        while (step_size > 0) {
            for (unsigned i = 0; i < step_size; ++i) {
                m_tree[index] = -1;
                --index;
            }
            index -= step_size;
            step_size /= 2;
        }

        m_stats.double_num_slots_time.stop();
    }

    //! Deactivate all players
    inline void clear()
    {
        std::fill(m_tree.begin(), m_tree.end(), -1);
    }

    inline void resize(unsigned num_players)
    {
        m_num_slots = (1 << ilog2_ceil(num_players));
        unsigned int treesize = (m_num_slots << 1) - 1;
        m_tree.resize(treesize, -1);
    }

    //! Deactivate all players and resize the tree.
    inline void resize_and_clear(unsigned num_players)
    {
        resize(num_players);
        clear();
    }

    inline void resize_and_rebuild(unsigned num_players)
    {
        //resize(num_players);
        resize_and_clear(num_players);
        for (unsigned i = 0; i < num_players; ++i)
            activate_without_replay(i);
        //for (unsigned i=num_players; i<m_num_slots; ++i)
        //    deactivate_without_replay(i);
        rebuild();
    }

    //! Build from winner tree from scratch.
    inline void rebuild()
    {
        for (int i = (int)(m_tree.size() / 2) - 1; i >= 0; --i) {
            const int lc = i * 2 + 1;
            const int rc = i * 2 + 2;
            int winner;

            if (m_tree[lc] < 0)
                winner = m_tree[rc];
            else if (m_tree[rc] < 0)
                winner = m_tree[lc];
            else if (m_less(m_tree[lc], m_tree[rc]))
                winner = m_tree[lc];
            else
                winner = m_tree[rc];

            m_tree[i] = winner;
        }
    }

    //! Returns a readable representation of the winner tree as string.
    std::string to_string() const
    {
        std::ostringstream ss;
        int levelsize = 1;
        int j = 0;
        for (unsigned int i = 0; i < m_tree.size(); ++i) {
            if ((int)i >= j + levelsize) {
                ss << "\n";
                j = i;
                levelsize *= 2;
            }
            ss << m_tree[i] << " ";
        }
        ss << "\n";
        return ss.str();
    }

    //! Print statistical data.
    void print_stats() const
    {
        STXXL_VARDUMP(m_num_slots);
        STXXL_MSG(m_stats);
    }
};

STXXL_END_NAMESPACE

#endif // !STXXL_COMMON_WINNER_TREE_HEADER

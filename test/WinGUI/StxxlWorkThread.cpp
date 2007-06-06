// StxxlWorkThread.cpp : implementation file
//

#include "stdafx.h"
#include "WinGUI.h"
#include "StxxlWorkThread.h"
#include ".\stxxlworkthread.h"
#include "WorkDialog.h"
#include "WinGUIDlg.h"

#include "UserTypes.h"


// CStxxlWorkThread

IMPLEMENT_DYNCREATE(CStxxlWorkThread, CWinThread)

CStxxlWorkThread::CStxxlWorkThread()
{ }

CStxxlWorkThread::~CStxxlWorkThread()
{ }

BOOL CStxxlWorkThread::InitInstance()
{
    // TODO:  perform and per-thread initialization here
    return TRUE;
}

int CStxxlWorkThread::ExitInstance()
{
    // TODO:  perform any per-thread cleanup here
    return CWinThread::ExitInstance();
}

BEGIN_MESSAGE_MAP(CStxxlWorkThread, CWinThread)
END_MESSAGE_MAP()


// CStxxlWorkThread message handlers

template <class T>
void CStxxlWorkThread::FillRandom(CString & path, LONGLONG filesize, T dummy)
{
    stxxl::wincall_file myfile(LPCSTR(dlg->m_Filename), stxxl::file::CREAT |
                               stxxl::file::DIRECT | stxxl::file::RDWR);
    stxxl::int64 vectorsize = dlg->m_Filesize * 4096 / sizeof(T);

    stxxl::vector<T> myvector(&myfile);
    myvector.resize(vectorsize);
    stxxl::generate(myvector.begin(), myvector.end(), my_rnd(), 4);
}

template <class T>
void CStxxlWorkThread::FillValue(CString & path, LONGLONG filesize, int value, T dummy)
{
    stxxl::wincall_file myfile(LPCSTR(dlg->m_Filename), stxxl::file::CREAT |
                               stxxl::file::DIRECT | stxxl::file::RDWR);
    stxxl::int64 vectorsize = dlg->m_Filesize * 4096 / sizeof(T);

    stxxl::vector<T> myvector(&myfile);
    myvector.resize(vectorsize);
    std::fill(myvector.begin(), myvector.end(), value);
}

template <class Cmp, class T>
void CStxxlWorkThread::Sort(CString & path, unsigned mem_usage, Cmp cmp, T dummy)
{
    stxxl::wincall_file myfile(LPCSTR(dlg->m_Filename), stxxl::file::CREAT |
                               stxxl::file::DIRECT | stxxl::file::RDWR);

    stxxl::vector<T> myvector(&myfile);
    if ((myvector.size() * sizeof(T)) % 4096)
    {
        AfxMessageBox("File can not be sorted because it is accessed with direct I/O\n"
                      "and its size is not a multiple of 4KBytes.");
        return;
    }

    stxxl::sort(myvector.begin(), myvector.end(), cmp, mem_usage * 1024 * 1024);
}

template <class Cmp, class T>
void CStxxlWorkThread::CheckOrder(CString & path, Cmp cmp, T dummy)
{
    stxxl::wincall_file myfile(LPCSTR(dlg->m_Filename), stxxl::file::CREAT |
                               stxxl::file::DIRECT | stxxl::file::RDWR);

    stxxl::vector<T> myvector(&myfile);
    if ((myvector.size() * sizeof(T)) % 4096)
    {
        AfxMessageBox("File can not be sorted because it is accessed with direct I/O\n"
                      "and its size is not a multiple of 4KBytes.");
        return;
    }

    bool sorted = stxxl::is_sorted(myvector.begin(), myvector.end(), cmp);
    if (sorted)
        AfxMessageBox("The file is sorted");

    else
        AfxMessageBox("The file is NOT sorted");

}

template <class T>
void CStxxlWorkThread::DelDuplicates(CString & path, unsigned mem_usage, T dummy)
{
    stxxl::wincall_file myfile(LPCSTR(dlg->m_Filename), stxxl::file::CREAT |
                               stxxl::file::DIRECT | stxxl::file::RDWR);

    stxxl::vector<T> myvector(&myfile);
    if ((myvector.size() * sizeof(T)) % 4096)
    {
        AfxMessageBox("File can not be sorted because it is accessed with direct I/O\n"
                      "and its size is not a multiple of 4KBytes.");
        return;
    }

    stxxl::sort(myvector.begin(), myvector.end(), LessCmp<T>(), mem_usage * 1024 * 1024);
    std::unique(myvector.begin(), myvector.end());
}


int CStxxlWorkThread::Run()
{
    // TODO: Add your specialized code here and/or call the base class

    switch (taskid)
    {
    case FILL_RANDOM:
        if (dlg->m_Format1)
            FillRandom(dlg->m_Filename, dlg->m_Filesize, type2());

        else
            FillRandom(dlg->m_Filename, dlg->m_Filesize, type1());

        break;
    case FILL_VALUE:
        if (dlg->m_Format1)
            FillValue(dlg->m_Filename, dlg->m_Filesize, dlg->m_Key, type2());

        else
            FillValue(dlg->m_Filename, dlg->m_Filesize, dlg->m_Key, type1());

        break;
    case SORT_ASC:
        if (dlg->m_Format1)
            Sort(dlg->m_Filename, dlg->m_MemUsage, LessCmp<type2>(), type2());

        else
            Sort(dlg->m_Filename, dlg->m_MemUsage, LessCmp<type1>(), type1());

        break;
    case SORT_DESC:
        if (dlg->m_Format1)
            Sort(dlg->m_Filename, dlg->m_MemUsage, GreaterCmp<type2>(), type2());

        else
            Sort(dlg->m_Filename, dlg->m_MemUsage, GreaterCmp<type1>(), type1());

        break;
    case CHECK_ASC:
        if (dlg->m_Format1)
            CheckOrder(dlg->m_Filename, LessCmp<type2>(), type2());

        else
            CheckOrder(dlg->m_Filename, LessCmp<type1>(), type1());

        break;
    case CHECK_DESC:
        if (dlg->m_Format1)
            CheckOrder(dlg->m_Filename, GreaterCmp<type2>(), type2());

        else
            CheckOrder(dlg->m_Filename, GreaterCmp<type1>(), type1());

        break;
    case DEL_DUP:
        if (dlg->m_Format1)
            DelDuplicates(dlg->m_Filename, dlg->m_MemUsage, type2());

        else
            DelDuplicates(dlg->m_Filename, dlg->m_MemUsage, type1());

        break;

    default:
        AfxMessageBox("Command not known or not implemented");
    };



    PostMessage(notifywindow->m_hWnd, WM_WORK_THREAD_FINISHED, 0, 0);

    m_bAutoDelete = TRUE;

    return CWinThread::Run();
}

CStxxlWorkThread::CStxxlWorkThread(int taskid_, CWinGUIDlg * dlg_, CWnd * notifywindow_) :
    taskid(taskid_), dlg(dlg_), notifywindow(notifywindow_)
{ }

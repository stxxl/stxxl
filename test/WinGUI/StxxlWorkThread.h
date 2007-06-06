#pragma once



// CStxxlWorkThread

class CWinGUIDlg;

class CStxxlWorkThread : public CWinThread
{
    DECLARE_DYNCREATE(CStxxlWorkThread)

protected:
    CStxxlWorkThread();               // protected constructor used by dynamic creation


public:
    virtual ~CStxxlWorkThread();

public:
    enum {  FILL_RANDOM,
            FILL_VALUE,
            SORT_ASC,
            SORT_DESC,
            CHECK_ASC,
            CHECK_DESC,
            DEL_DUP,
            CHECK_DUP};

    virtual BOOL InitInstance();
    virtual int ExitInstance();

    CStxxlWorkThread(int taskid, CWinGUIDlg * dlg, CWnd * notifywindow);
protected:
    DECLARE_MESSAGE_MAP()
public:
    virtual int Run();
protected:
    template <class T>
    void FillRandom(CString & path, LONGLONG filesize, T dummy);
    template <class T>
    void FillValue(CString & path, LONGLONG filesize, int value, T dummy);
    template <class Cmp, class T>
    void Sort(CString & path, unsigned mem_usage, Cmp cmp, T dummy);
    template <class Cmp, class T>
    void CheckOrder(CString & path, Cmp cmp, T dummy);
    template <class T>
    void DelDuplicates(CString & path, unsigned mem_usage, T dummy);

    int taskid;
    CWinGUIDlg * dlg;
    CWnd * notifywindow;
};



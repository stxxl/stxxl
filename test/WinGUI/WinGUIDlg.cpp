// WinGUIDlg.cpp : implementation file
//

#include "stdafx.h"
#include "WinGUI.h"
#include "WinGUIDlg.h"
#include ".\winguidlg.h"

#include "WorkDialog.h"
#include "StxxlWorkThread.h"
#include "UserTypes.h"

#ifdef _DEBUG
 #define new DEBUG_NEW
#endif


// CAboutDlg dialog used for App About

class CAboutDlg : public CDialog
{
public:
    CAboutDlg();

// Dialog Data
    enum { IDD = IDD_ABOUTBOX };

protected:
    virtual void DoDataExchange(CDataExchange * pDX);       // DDX/DDV support

// Implementation
protected:
    DECLARE_MESSAGE_MAP()
};

CAboutDlg::CAboutDlg() : CDialog(CAboutDlg::IDD)
{ }

void CAboutDlg::DoDataExchange(CDataExchange * pDX)
{
    CDialog::DoDataExchange(pDX);
}

BEGIN_MESSAGE_MAP(CAboutDlg, CDialog)
END_MESSAGE_MAP()


// CWinGUIDlg dialog



CWinGUIDlg::CWinGUIDlg(CWnd * pParent /*=NULL*/)
    : CDialog(CWinGUIDlg::IDD, pParent)
      , m_Filename(_T("c:\\myfile"))
      , m_Format1(FALSE)
      , m_Filesize(LONGLONG(1024) * LONGLONG(512 / 4)) // 1 GiB
      , m_Key(555)
      , m_MemUsage(128)
      , m_FilesizeMB(0)
{
    m_FilesizeMB = m_Filesize / 256;
    m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
}

void CWinGUIDlg::DoDataExchange(CDataExchange * pDX)
{
    CDialog::DoDataExchange(pDX);
    DDX_Text(pDX, IDC_FILENAMEEDIT, m_Filename);
    DDX_Radio(pDX, IDC_FORMAT1RADIO, m_Format1);
    DDX_Text(pDX, IDC_FILESIZEEDIT, m_Filesize);
    DDX_Text(pDX, IDC_FILESIZEEDITMB, m_FilesizeMB);
    DDX_Text(pDX, IDC_INTEGERKEYEDIT, m_Key);
    DDX_Text(pDX, IDC_MEMUSAGEEDIT, m_MemUsage);
}

BEGIN_MESSAGE_MAP(CWinGUIDlg, CDialog)
ON_WM_SYSCOMMAND()
ON_WM_PAINT()
ON_WM_QUERYDRAGICON()
//}}AFX_MSG_MAP
ON_BN_CLICKED(IDCANCEL, OnBnClickedCancel)
ON_BN_CLICKED(IDOK, OnBnClickedOk)
ON_BN_CLICKED(IDC_FILENAMEBUTTON, OnBnClickedFilenamebutton)
ON_BN_CLICKED(IDC_FILLRANDOMBUTTON, OnBnClickedFillrandombutton)
ON_EN_UPDATE(IDC_FILESIZEEDIT, OnEnUpdateFilesizeedit)
//	ON_EN_CHANGE(IDC_FILESIZEEDIT, OnEnChangeFilesizeedit)
ON_BN_CLICKED(IDC_FILLVALUEBUTTON, OnBnClickedFillvaluebutton)
ON_BN_CLICKED(IDC_SORTASCBUTTON, OnBnClickedSortascbutton)
ON_BN_CLICKED(IDC_SORTDESCBUTTON4, OnBnClickedSortdescbutton4)
ON_BN_CLICKED(IDC_CHECKASCBUTTON, OnBnClickedCheckascbutton)
ON_BN_CLICKED(IDC_CHECKDESCBUTTON, OnBnClickedCheckdescbutton)
ON_BN_CLICKED(IDC_REMDUPLBUTTON, OnBnClickedRemduplbutton)
//ON_BN_CLICKED(IDC_CHECKDUPLBUTTON, OnBnClickedCheckduplbutton)
END_MESSAGE_MAP()


// CWinGUIDlg message handlers

BOOL CWinGUIDlg::OnInitDialog()
{
    CDialog::OnInitDialog();

    // Add "About..." menu item to system menu.

    // IDM_ABOUTBOX must be in the system command range.
    ASSERT((IDM_ABOUTBOX & 0xFFF0) == IDM_ABOUTBOX);
    ASSERT(IDM_ABOUTBOX < 0xF000);

    CMenu * pSysMenu = GetSystemMenu(FALSE);
    if (pSysMenu != NULL)
    {
        CString strAboutMenu;
        strAboutMenu.LoadString(IDS_ABOUTBOX);
        if (!strAboutMenu.IsEmpty())
        {
            pSysMenu->AppendMenu(MF_SEPARATOR);
            pSysMenu->AppendMenu(MF_STRING, IDM_ABOUTBOX, strAboutMenu);
        }
    }

    // Set the icon for this dialog.  The framework does this automatically
    //  when the application's main window is not a dialog
    SetIcon(m_hIcon, TRUE);                     // Set big icon
    SetIcon(m_hIcon, FALSE);                    // Set small icon

    // TODO: Add extra initialization here

    return TRUE;      // return TRUE  unless you set the focus to a control
}

void CWinGUIDlg::OnSysCommand(UINT nID, LPARAM lParam)
{
    if ((nID & 0xFFF0) == IDM_ABOUTBOX)
    {
        CAboutDlg dlgAbout;
        dlgAbout.DoModal();
    }
    else
    {
        CDialog::OnSysCommand(nID, lParam);
    }
}

// If you add a minimize button to your dialog, you will need the code below
//  to draw the icon.  For MFC applications using the document/view model,
//  this is automatically done for you by the framework.

void CWinGUIDlg::OnPaint()
{
    if (IsIconic())
    {
        CPaintDC dc(this);         // device context for painting

        SendMessage(WM_ICONERASEBKGND, reinterpret_cast<WPARAM>(dc.GetSafeHdc()), 0);

        // Center icon in client rectangle
        int cxIcon = GetSystemMetrics(SM_CXICON);
        int cyIcon = GetSystemMetrics(SM_CYICON);
        CRect rect;
        GetClientRect(&rect);
        int x = (rect.Width() - cxIcon + 1) / 2;
        int y = (rect.Height() - cyIcon + 1) / 2;

        // Draw the icon
        dc.DrawIcon(x, y, m_hIcon);
    }
    else
    {
        CDialog::OnPaint();
    }
}

// The system calls this function to obtain the cursor to display while the user drags
//  the minimized window.
HCURSOR CWinGUIDlg::OnQueryDragIcon()
{
    return static_cast<HCURSOR>(m_hIcon);
}

void CWinGUIDlg::OnBnClickedCancel()
{
    // TODO: Add your control notification handler code here
    OnCancel();
}

void CWinGUIDlg::OnBnClickedOk()
{
    // TODO: Add your control notification handler code here
    OnOK();
}

void CWinGUIDlg::OnBnClickedFilenamebutton()
{
    // TODO: Add your control notification handler code here
    UpdateData();
    CFileDialog FileDialog(TRUE, NULL, LPCTSTR(m_Filename), OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT, 0, this);

    if (FileDialog.DoModal() == IDOK)
    {
        m_Filename = FileDialog.GetPathName();
        UpdateData(FALSE);
    }
}

void CWinGUIDlg::OnBnClickedFillrandombutton()
{
    // TODO: Add your control notification handler code here
    UpdateData();
    CWorkDialog WorkDialog;
    CStxxlWorkThread * Thread = new CStxxlWorkThread(CStxxlWorkThread::FILL_RANDOM, this, &WorkDialog);
    WorkDialog.SetThread(Thread);
    WorkDialog.DoModal();
}

void CWinGUIDlg::OnEnUpdateFilesizeedit()
{
    // TODO:  If this is a RICHEDIT control, the control will not
    // send this notification unless you override the CDialog::OnInitDialog()
    // function to send the EM_SETEVENTMASK message to the control
    // with the ENM_UPDATE flag ORed into the lParam mask.

    // TODO:  Add your control notification handler code here
    UpdateData();
    m_FilesizeMB = m_Filesize / 256;
    UpdateData(FALSE);
}

//void CWinGUIDlg::OnEnChangeFilesizeedit()
//{
//	// TODO:  If this is a RICHEDIT control, the control will not
//	// send this notification unless you override the CDialog::OnInitDialog()
//	// function and call CRichEditCtrl().SetEventMask()
//	// with the ENM_CHANGE flag ORed into the mask.
//
//	// TODO:  Add your control notification handler code here
//
//}

void CWinGUIDlg::OnBnClickedFillvaluebutton()
{
    // TODO: Add your control notification handler code here
    UpdateData();
    CWorkDialog WorkDialog;
    CStxxlWorkThread * Thread = new CStxxlWorkThread(CStxxlWorkThread::FILL_VALUE, this, &WorkDialog);
    WorkDialog.SetThread(Thread);
    WorkDialog.DoModal();
}

void CWinGUIDlg::OnBnClickedSortascbutton()
{
    // TODO: Add your control notification handler code here
    UpdateData();
    CWorkDialog WorkDialog;
    CStxxlWorkThread * Thread = new CStxxlWorkThread(CStxxlWorkThread::SORT_ASC, this, &WorkDialog);
    WorkDialog.SetThread(Thread);
    WorkDialog.DoModal();
}

void CWinGUIDlg::OnBnClickedSortdescbutton4()
{
    // TODO: Add your control notification handler code here
    UpdateData();
    CWorkDialog WorkDialog;
    CStxxlWorkThread * Thread = new CStxxlWorkThread(CStxxlWorkThread::SORT_DESC, this, &WorkDialog);
    WorkDialog.SetThread(Thread);
    WorkDialog.DoModal();
}

void CWinGUIDlg::OnBnClickedCheckascbutton()
{
    // TODO: Add your control notification handler code here
    UpdateData();
    CWorkDialog WorkDialog;
    CStxxlWorkThread * Thread = new CStxxlWorkThread(CStxxlWorkThread::CHECK_ASC, this, &WorkDialog);
    WorkDialog.SetThread(Thread);
    WorkDialog.DoModal();
}



void CWinGUIDlg::OnBnClickedCheckdescbutton()
{
    // TODO: Add your control notification handler code here
    UpdateData();
    CWorkDialog WorkDialog;
    CStxxlWorkThread * Thread = new CStxxlWorkThread(CStxxlWorkThread::CHECK_DESC, this, &WorkDialog);
    WorkDialog.SetThread(Thread);
    WorkDialog.DoModal();
}

void CWinGUIDlg::OnBnClickedRemduplbutton()
{
    // TODO: Add your control notification handler code here
    UpdateData();
    CWorkDialog WorkDialog;
    CStxxlWorkThread * Thread = new CStxxlWorkThread(CStxxlWorkThread::DEL_DUP, this, &WorkDialog);
    WorkDialog.SetThread(Thread);
    WorkDialog.DoModal();
}

//void CWinGUIDlg::OnBnClickedCheckduplbutton()
//{
//	// TODO: Add your control notification handler code here
//	UpdateData();
//	CWorkDialog WorkDialog;
//	CStxxlWorkThread * Thread = new CStxxlWorkThread(CStxxlWorkThread::CHECK_DUP,this,&WorkDialog);
//	WorkDialog.SetThread(Thread);
//	WorkDialog.DoModal();
//}

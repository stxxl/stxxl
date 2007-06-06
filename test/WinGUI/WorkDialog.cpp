// WorkDialog.cpp : implementation file
//

#include "stdafx.h"
#include "WinGUI.h"
#include "WorkDialog.h"
#include ".\workdialog.h"


// CWorkDialog dialog

IMPLEMENT_DYNAMIC(CWorkDialog, CDialog)
CWorkDialog::CWorkDialog(CWnd * pParent /*=NULL*/)
    : CDialog(CWorkDialog::IDD, pParent)
      , m_Message(_T("Working ...")),
      thread(NULL)
{ }

CWorkDialog::~CWorkDialog()
{ }

void CWorkDialog::DoDataExchange(CDataExchange * pDX)
{
    CDialog::DoDataExchange(pDX);
    DDX_Text(pDX, IDC_MESSAGE1, m_Message);
}


BEGIN_MESSAGE_MAP(CWorkDialog, CDialog)
ON_BN_CLICKED(IDOK, OnBnClickedOk)
ON_BN_CLICKED(IDCANCEL, OnBnClickedCancel)
ON_MESSAGE(WM_WORK_THREAD_FINISHED, OnThreadFinished)
ON_WM_CREATE()
ON_WM_TIMER()
END_MESSAGE_MAP()


// CWorkDialog message handlers

void CWorkDialog::OnBnClickedOk()
{
    // TODO: Add your control notification handler code here
    //OnOK();
}

void CWorkDialog::OnBnClickedCancel()
{
    // TODO: Add your control notification handler code here
    //OnCancel();
}


afx_msg LRESULT CWorkDialog::OnThreadFinished(WPARAM wparam, LPARAM lparam)
{
    EndDialog(IDOK);
    return 0;
}

void CWorkDialog::SetThread(CWinThread * thread_)
{
    thread = thread_;
}

int CWorkDialog::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
    if (CDialog::OnCreate(lpCreateStruct) == -1)
        return -1;


    ASSERT(thread);

    thread->CreateThread();

    SetTimer(WORK_TIMER_ID, 400, NULL);


    return 0;
}


void CWorkDialog::OnTimer(UINT nIDEvent)
{
    // TODO: Add your message handler code here and/or call default
    if (nIDEvent == WORK_TIMER_ID)
    {
        if (m_Message == "")
            m_Message = "Working ...";

        else
            m_Message = "";

        UpdateData(FALSE);
    }

    CDialog::OnTimer(nIDEvent);
}

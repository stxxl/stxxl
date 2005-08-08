#pragma once

#define WM_WORK_THREAD_FINISHED (WM_USER+0x101)
#define WORK_TIMER_ID (1)

// CWorkDialog dialog

class CWorkDialog : public CDialog
{
	DECLARE_DYNAMIC(CWorkDialog)

public:
	CWorkDialog(CWnd* pParent = NULL);   // standard constructor
	virtual ~CWorkDialog();

// Dialog Data
	enum { IDD = IDD_WORKDIALOG };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

	DECLARE_MESSAGE_MAP()
public:
	afx_msg void OnBnClickedOk();
	afx_msg void OnBnClickedCancel();
	CString m_Message;
protected:
	CWinThread * thread;
public:
	afx_msg LRESULT OnThreadFinished(WPARAM wparam , LPARAM lparam);
	void SetThread(CWinThread * thread_);
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg void OnTimer(UINT nIDEvent);
};

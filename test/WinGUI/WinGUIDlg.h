// WinGUIDlg.h : header file
//

#pragma once


// CWinGUIDlg dialog
class CWinGUIDlg : public CDialog
{
// Construction
public:
	CWinGUIDlg(CWnd* pParent = NULL);	// standard constructor

// Dialog Data
	enum { IDD = IDD_WINGUI_DIALOG };

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV support


// Implementation
protected:
	HICON m_hIcon;

	// Generated message map functions
	virtual BOOL OnInitDialog();
	afx_msg void OnSysCommand(UINT nID, LPARAM lParam);
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();
	DECLARE_MESSAGE_MAP()
public:
	afx_msg void OnBnClickedCancel();
	afx_msg void OnBnClickedOk();
	CString m_Filename;
	BOOL m_Format1;
	LONGLONG m_Filesize;
	int m_Key;
	DWORD m_MemUsage;
	afx_msg void OnBnClickedFilenamebutton();
	afx_msg void OnBnClickedFillrandombutton();
	long m_FilesizeMB;
	afx_msg void OnEnUpdateFilesizeedit();
//	afx_msg void OnEnChangeFilesizeedit();
	afx_msg void OnBnClickedFillvaluebutton();
	afx_msg void OnBnClickedSortascbutton();
	afx_msg void OnBnClickedSortdescbutton4();
	afx_msg void OnBnClickedCheckascbutton();
	afx_msg void OnBnClickedCheckdescbutton();
	afx_msg void OnBnClickedRemduplbutton();
//	afx_msg void OnBnClickedCheckduplbutton();
};

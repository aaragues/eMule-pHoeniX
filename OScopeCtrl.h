#pragma once

/////////////////////////////////////////////////////////////////////////////
// COScopeCtrl window

class COScopeCtrl : public CWnd
{
public:
	COScopeCtrl(int NTrends = 1);
	virtual ~COScopeCtrl();
	
	virtual BOOL Create(DWORD dwStyle, const RECT& rect, CWnd* pParentWnd, UINT nID = NULL);
	
	// Attributes
	//  Added parameters: bool bUseTrendRatio = true (AppendPoints and AppendEmptyPoints)
	//				Added public function: SetTrendRatio
	void AppendPoints(double dNewPoint[], bool bInvalidate = true, bool bAdd2List = true, bool bUseTrendRatio = true);
	void AppendEmptyPoints(double dNewPoint[], bool bInvalidate = true, bool bAdd2List = true, bool bUseTrendRatio = true);
	void SetTrendRatio(int iTrend, unsigned int iRatio = 1);
	void SetLegendLabel(CString string,int iTrend);
	void SetBarsPlot(bool BarsPlot,int iTrend);
	void SetRange(double dLower, double dUpper, int iTrend = 0);
	void SetRanges(double dLower, double dUpper);
	void SetXUnits(CString string, CString XMin = _T(""), CString XMax = _T(""));
	void SetYUnits(CString string, CString YMin = _T(""), CString YMax = _T(""));
	void SetGridColor(COLORREF color);
	void SetPlotColor(COLORREF color, int iTrend = 0);
	COLORREF GetPlotColor(int iTrend = 0);
	void SetBackgroundColor(COLORREF color);
	void InvalidateCtrl(bool deleteGraph = true);
	void DrawPoint();
	void Reset();
	int ReCreateGraph(void);
	void GetPlotRect(CRect& rPlotRect) { rPlotRect = m_rectPlot; }

	bool ready;
	bool drawBars;
	bool autofitYscale;
	int m_nXGrids;
	int m_nYGrids;
	int m_nXPartial;
	int m_nShiftPixels;         // amount to shift with each new point 
	int m_nTrendPoints;			// when you set this to > 0, then plot will
	int m_nMaxPointCnt;
	// contain that much points (regardless of the
	// Trend/control) width drawn on screen !!!
	
	// Otherwise, if this is -1 (which is default),
	// m_nShiftPixels will be in use
	int m_nYDecimals;
	
	typedef struct m_str_struct 
	{	
		CString XUnits, XMin, XMax;
		CString YUnits, YMin, YMax;
	} m_str_t;
	m_str_t m_str;
	
	COLORREF m_crBackColor;        // background color
	COLORREF m_crGridColor;        // grid color
	
	typedef struct PlotDataStruct 
	{
		// [TPT]
		// Maella -Code Improvement-
		PlotDataStruct() : lstPoints(1024+1) {}; // see m_nMaxPointCnt
		// Maella end

		COLORREF crPlotColor;       // data plot color  
		CPen   penPlot;
		double dCurrentPosition;    // current position
		double dPreviousPosition;   // previous position
		int nPrevY;
		double dLowerLimit;         // lower bounds
		double dUpperLimit;         // upper bounds
		double dRange;				// = UpperLimit - LowerLimit
		double dVerticalFactor;
		// Optional variable to set a ratio for a given "trend".
		//				The purpose here is to better implement the customizable
		//				active connections ratio, so that points are redrawn correctly
		//				when the ratio is changed, rather than having all previous points
		//				no longer match up with the new ratio.
		int		iTrendRatio;
		CString LegendLabel;
		bool BarsPlot;
		CList<double> lstPoints;
	} PlotData_t;
	
	// Generated message map functions
protected:
	int m_NTrends;
	
		struct CustShiftStruct 
	{		// when m_nTrendPoints > 0, this structure will contain needed vars
		int m_nRmndr;				// reminder after dividing m_nWidthToDo/m_nPointsToDo
		int m_nWidthToDo;
		int m_nPointsToDo;
	} CustShift;
	
	PlotData_t *m_PlotData; // !!! !!!
	
	int m_nClientHeight;
	int m_nClientWidth;
	int m_nPlotHeight;
	int m_nPlotWidth;
	
	CRect  m_rectClient;
	CRect  m_rectPlot;
	CBrush m_brushBack;
	
	CDC     m_dcGrid;
	CDC     m_dcPlot;
	CBitmap *m_pbitmapOldGrid;
	CBitmap *m_pbitmapOldPlot;
	CBitmap m_bitmapGrid;
	CBitmap m_bitmapPlot;

	bool m_bDoUpdate;
	UINT m_nRedrawTimer;

	DECLARE_MESSAGE_MAP()
	afx_msg void OnPaint();
	afx_msg void OnSize(UINT nType, int cx, int cy); 
	afx_msg void OnLButtonDblClk(UINT nFlags, CPoint point);
	afx_msg void OnTimer(UINT nIDEvent);
};

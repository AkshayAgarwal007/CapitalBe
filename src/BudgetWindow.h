#ifndef BUDGETWIN_H
#define BUDGETWIN_H

#include <Window.h>
#include <Box.h>
#include <TextControl.h>
#include <ListView.h>
#include <ListItem.h>
#include <ScrollView.h>
#include <time.h>
#include "ColumnListView.h"
#include <Button.h>
#include <MenuField.h>
#include <Menu.h>
#include <MenuItem.h>
#include <StringView.h>
#include <RadioButton.h>
#include "ReportGrid.h"
#include "Budget.h"

class CurrencyBox;

class BudgetWindow : public BWindow
{
public:
	BudgetWindow(const BRect &frame);
	~BudgetWindow(void);
	void MessageReceived(BMessage *msg);

private:
	void BuildStatsAndEditor(void);
	void BuildBudgetSummary(void);
	void BuildCategoryList(void);
	
	void RefreshCategories(void);
	void RefreshBudgetSummary(void);
	void RefreshBudgetGrid(void);
	void GenerateBudget(const bool &zero);
	void CalcStats(const char *cat, Fixed &high, Fixed &low, Fixed &avg);
	void HandleCategorySelection(void);
	void SetPeriod(const BudgetPeriod &period);
	
	BMenuBar *fBar;
	BView *fBackView;
	
	BColumnListView *fCategoryList;
	BRow	*fIncomeRow,
			*fSpendingRow;
	
	BColumnListView *fBudgetSummary;
	BRow	*fSummaryIncomeRow,
			*fSummarySpendingRow,
			*fSummaryTotalRow;
	
	BBox			*fCatBox;
	CurrencyBox		*fAmountBox;
	BStringView		*fAmountLabel;
	BRadioButton	*fMonthly,
					*fWeekly,
					*fQuarterly,
					*fAnnually;
	
	BColumnListView *fCatStat;
	BRow	*fStatAverageRow,
			*fStatHighestRow,
			*fStatLowestRow;
	
	ReportGrid	fIncomeGrid,
				fSpendingGrid;
};

#endif

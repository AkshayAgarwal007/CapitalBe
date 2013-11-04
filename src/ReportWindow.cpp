#include "ReportWindow.h"
#include <View.h>
#include <StringView.h>
#include <Bitmap.h>
#include <TranslationUtils.h>
#include "ReportGrid.h"
#include "DateBox.h"
#include "StickyDrawButton.h"
#include "Layout.h"
#include "TimeSupport.h"
#include "ObjectList.h"
#include "Database.h"
#include "ColumnTypes.h"
#include "HelpButton.h"
#include "Preferences.h"
#include "MsgDefs.h"

int compare_stringitem(const void *item1, const void *item2);

enum
{
	M_REPORT_CASH_FLOW='csfl',
	M_REPORT_NET_WORTH,
	M_REPORT_TRANSACTIONS,
	M_REPORT_BUDGET,
	
	M_START_DATE_CHANGED,
	M_END_DATE_CHANGED,
	M_CATEGORIES_CHANGED,
	
	M_SUBTOTAL_NONE,
	M_SUBTOTAL_MONTH,
	M_SUBTOTAL_QUARTER,
	M_SUBTOTAL_YEAR,
	
	M_TOGGLE_ACCOUNT,
	M_TOGGLE_GRAPH
};


ReportWindow::ReportWindow(BRect frame)
 : BWindow(frame,TRANSLATE("Reports"), B_DOCUMENT_WINDOW, B_ASYNCHRONOUS_CONTROLS),
 	fSubtotalMode(SUBTOTAL_NONE),
 	fReportMode(REPORT_CASH_FLOW),
 	fStartDate(GetCurrentYear()),
 	fEndDate(GetCurrentDate()),
 	fTitleFont(be_bold_font),
 	fHeaderFont(be_plain_font)
{
	BString temp;
	
	SetSizeLimits(520,30000,260,30000);
	fHeaderFont.SetFace(B_ITALIC_FACE);
	
	BView *view = new BView(Bounds(),"back",B_FOLLOW_ALL, B_WILL_DRAW);
//	view->SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));
	view->SetViewColor(240,240,240);
	AddChild(view);
	
	BMenu *reportmenu = new BMenu(TRANSLATE("Reports"));
	reportmenu->SetLabelFromMarked(true);
	
	// TODO: Re-enable the Budget report	
//	reportmenu->AddItem(new BMenuItem(TRANSLATE("Budget"), new BMessage(M_REPORT_BUDGET)));
	temp << TRANSLATE("Income") << " / " << TRANSLATE("Spending");
	reportmenu->AddItem(new BMenuItem(temp.String(), new BMessage(M_REPORT_CASH_FLOW)));
	reportmenu->AddItem(new BMenuItem(TRANSLATE("Total Worth"), new BMessage(M_REPORT_NET_WORTH)));
	reportmenu->AddItem(new BMenuItem(TRANSLATE("Transactions"), new BMessage(M_REPORT_TRANSACTIONS)));
	reportmenu->SetRadioMode(true);
	reportmenu->ItemAt(0L)->SetMarked(true);

	fReportMode = REPORT_BUDGET;
 
	BRect r(10,10,reportmenu->StringWidth(temp.String())+45,60);
	temp = TRANSLATE("Reports"); temp += ": ";
	BStringView *sv = new BStringView(r,"reportsv",temp.String());
	sv->ResizeToPreferred();
	view->AddChild(sv);
	
	r = sv->Frame();
	r.OffsetBy(0,r.Height());
	r.right = r.left + reportmenu->StringWidth(temp.String()) + 45;
	fReportField = new BMenuField(r,"reportfield","",reportmenu);
	fReportField->SetDivider(0);
	view->AddChild(fReportField);
	
	r = fReportField->Frame();
	r.OffsetBy(0,r.Height() + 10);
	
	temp = TRANSLATE("Accounts"); temp += ": ";
	sv = new BStringView(BRect(r.left,r.top,r.left+1,r.top+1),"accountsv",temp.String());
	sv->ResizeToPreferred();
	r = sv->Frame();
	view->AddChild(sv);
	
	r.OffsetBy(0,r.Height());
	r.right = r.left + be_plain_font->StringWidth("AccountName");
	r.bottom = r.top + (r.Height() * 3);
	fAccountList = new BListView(r,"reportaccountlist",B_MULTIPLE_SELECTION_LIST);
	
	BScrollView *scrollview = new BScrollView("accountscroller",fAccountList,
												B_FOLLOW_LEFT | B_FOLLOW_TOP,0,
												false,true);
	view->AddChild(scrollview);
	
	// This is disabled because otherwise the report is rendered once for each
	// account added when the report window is shown initially
//	fAccountList->SetSelectionMessage(new BMessage(M_TOGGLE_ACCOUNT));
	
	r.top = scrollview->Frame().bottom + 10;
	r.right = sv->StringWidth("Current Quarter") + 45;
	r.bottom = r.top + reportmenu->Frame().Height();
	
	temp = TRANSLATE("Subtotal"); temp += ":";
	sv = new BStringView(r,"subtotalsv",temp.String());
	sv->ResizeToPreferred();
	view->AddChild(sv);
	
	r.OffsetTo(r.left, sv->Frame().top + sv->Frame().Height());
	
	BMenu *subtotalmenu = new BMenu(TRANSLATE("Subtotal"));
	subtotalmenu->AddItem(new BMenuItem(TRANSLATE("None"),new BMessage(M_SUBTOTAL_NONE)));
	subtotalmenu->AddItem(new BMenuItem(TRANSLATE("Month"),new BMessage(M_SUBTOTAL_MONTH)));
	subtotalmenu->AddItem(new BMenuItem(TRANSLATE("Quarter"),new BMessage(M_SUBTOTAL_QUARTER)));
	subtotalmenu->AddItem(new BMenuItem(TRANSLATE("Year"),new BMessage(M_SUBTOTAL_YEAR)));
	subtotalmenu->SetLabelFromMarked(true);
	subtotalmenu->SetRadioMode(true);
	subtotalmenu->ItemAt(0)->SetMarked(true);
	
	fSubtotalField = new BMenuField(r,"subtotalfield","",subtotalmenu);
	fSubtotalField->SetDivider(0);
	view->AddChild(fSubtotalField);
	
	prefsLock.Lock();
	BString reporthelp = gAppPath;
	prefsLock.Unlock();
	reporthelp << "helpfiles/" << gCurrentLanguage->Name() << "/Report Window Help";
	HelpButton *help = new HelpButton(BPoint(r.right + 10,r.top),"reporthelp", reporthelp.String());
//	help->MoveTo(fSubtotalField->Frame().right, fSubtotalField->Frame().top);
	view->AddChild(help);
	
	r.OffsetTo(r.left, fSubtotalField->Frame().bottom + 10);
	temp = TRANSLATE("Categories"); temp += ": ";
	sv = new BStringView(r,"catsv",temp.String());
	sv->ResizeToPreferred();
	view->AddChild(sv);
	
	r = sv->Frame();
	r.OffsetBy(0,r.Height());
	r.bottom = Bounds().bottom - 13;
	r.right = r.left + fAccountList->Bounds().Width();
	
	fCategoryList = new BListView(r,"reportcattlist",B_MULTIPLE_SELECTION_LIST,
									B_FOLLOW_LEFT | B_FOLLOW_TOP_BOTTOM,
									B_WILL_DRAW | B_FULL_UPDATE_ON_RESIZE | B_NAVIGABLE);
	fCategoryScroller = new BScrollView("catscroller",fCategoryList,
										B_FOLLOW_LEFT | B_FOLLOW_TOP_BOTTOM,0,
										false,true);
	view->AddChild(fCategoryScroller);
	fCategoryList->SetSelectionMessage(new BMessage(M_CATEGORIES_CHANGED));
	fCategoryList->SetInvocationMessage(new BMessage(M_CATEGORIES_CHANGED));
	
	BString datestring;
	gDefaultLocale.DateToString(GetCurrentYear(),datestring);
	
	temp = TRANSLATE("Starting Date"); temp += ": ";
	
	r.left = fCategoryScroller->Frame().right + 10;
	r.top = 10;
	r.right = r.left + view->StringWidth(temp.String()) + view->StringWidth("00-00-0000") + 15;
	r.bottom = r.top + gTextViewHeight;
	
	fStartDateBox = new DateBox(r,"startdate",temp.String(),datestring.String(),
								new BMessage(M_START_DATE_CHANGED));
	fStartDateBox->SetDivider(fStartDateBox->StringWidth(temp.String()));
	fStartDateBox->SetDate(GetCurrentYear());
//	fStartDateBox->SetEscapeCancel(true);
	view->AddChild(fStartDateBox);
	fStartDateBox->GetFilter()->SetMessenger(new BMessenger(this));
	
	gDefaultLocale.DateToString(GetCurrentDate(),datestring);
	temp = TRANSLATE("Ending Date"); temp += ": ";
	
	r.OffsetTo(fStartDateBox->Frame().right + 10, r.top);
	r.right = r.left + view->StringWidth(temp.String()) + view->StringWidth("00-00-0000") + 15;
	fEndDateBox = new DateBox(r,"enddate",temp.String(),datestring.String(),
								new BMessage(M_END_DATE_CHANGED));
	fEndDateBox->SetDivider(fEndDateBox->StringWidth(temp.String()));
	fEndDateBox->SetDate(GetCurrentDate());
	view->AddChild(fEndDateBox);
	fEndDateBox->GetFilter()->SetMessenger(new BMessenger(this));

	help->MoveTo(Bounds().right - 10 - help->Frame().Width(),10 + 
					((fEndDateBox->Frame().Height() - 20)/2) );
		
	BBitmap *up, *down;
	BRect brect(0,0,16,16);
	up = BTranslationUtils::GetBitmap('PNG ',"BarGraphUp.png");
	if(!up)
		up = new BBitmap(brect,B_RGB32);
	down = BTranslationUtils::GetBitmap('PNG ',"BarGraphDown.png");
	if(!down)
		down = new BBitmap(brect,B_RGB32);
	
	brect.OffsetTo(Bounds().right - 10 - brect.Width(),10 + 
					((fEndDateBox->Frame().Height() - 16)/2) );
	fGraphButton = new StickyDrawButton(brect,"graphbutton",up,down,
										new BMessage(M_TOGGLE_GRAPH),
										B_FOLLOW_TOP | B_FOLLOW_RIGHT,B_WILL_DRAW);
	view->AddChild(fGraphButton);

// TODO: This needs to be unhidden when graph support is finally added
fGraphButton->Hide();

	r = Bounds().InsetByCopy(13,13);
	r.left = fCategoryScroller->Frame().right + 10;
	r.top += 30;
	r.bottom -= B_H_SCROLL_BAR_HEIGHT;
	r.right -= B_V_SCROLL_BAR_WIDTH;
	fGridView = new BColumnListView(r,"gridview",B_FOLLOW_ALL,B_WILL_DRAW, B_FANCY_BORDER);
	view->AddChild(fGridView);
	
	// Configuring to make it look good and not like a grid
	fGridView->SetColumnFlags(B_ALLOW_COLUMN_RESIZE);
	fGridView->SetSortingEnabled(false);
	fGridView->SetEditMode(false);
	
	rgb_color white = { 255,255,255,255 };
	fGridView->SetColor(B_COLOR_BACKGROUND,white);
	fGridView->SetColor(B_COLOR_ROW_DIVIDER,fGridView->Color(B_COLOR_BACKGROUND));
	fGridView->SetColor(B_COLOR_SELECTION,fGridView->Color(B_COLOR_BACKGROUND));
	fGridView->SetColor(B_COLOR_NON_FOCUS_SELECTION,fGridView->Color(B_COLOR_BACKGROUND));
	fGridView->SetColor(B_COLOR_SEPARATOR_BORDER,fGridView->Color(B_COLOR_BACKGROUND));
	fGridView->SetColor(B_COLOR_SEPARATOR_LINE,fGridView->Color(B_COLOR_BACKGROUND));
	
	fGraphView = new BView(r,"As Graph",B_FOLLOW_ALL,B_WILL_DRAW);
	view->AddChild(fGraphView);
	fGraphView->Hide();
		
	gDatabase.AddObserver(this);
	for(int32 i=0; i<gDatabase.CountAccounts(); i++)
	{
		Account *acc = gDatabase.AccountAt(i);
		if(!acc)
			continue;
		
		AddAccount(acc);
	}
	
	fAccountList->SortItems(compare_stringitem);
	fCategoryList->SortItems(compare_stringitem);
	
	for(int32 i=0; i<fAccountList->CountItems(); i++)
	{
		BStringItem *item = (BStringItem*)fAccountList->ItemAt(i);
		if(!item)
			continue;
		
		Account *itemaccount = gDatabase.AccountByName(item->Text());
		if(itemaccount && (!itemaccount->IsClosed()))
			fAccountList->Select(i,true);
	}
//	fAccountList->Select(0,fAccountList->CountItems()-1,true);
	fCategoryList->Select(0,fCategoryList->CountItems()-1,true);
	
	// Set up the scrollbars
	FrameResized(Bounds().Width(),Bounds().Height());
	
	CalcCategoryString();
	
	fReportField->MakeFocus(true);
	
	fAccountList->SetSelectionMessage(new BMessage(M_TOGGLE_ACCOUNT));
}

void ReportWindow::HandleNotify(const uint64 &value, const BMessage *msg)
{
	Lock();
	
	if(value & WATCH_ACCOUNT)
	{
		Account *acc;
		if(msg->FindPointer("item",(void**)&acc)!=B_OK)
		{
			Unlock();
			return;
		}
		
		if(value & WATCH_CREATE)
		{
			AddAccount(acc);
		}
		else
		if(value & WATCH_DELETE)
		{
		}
		else
		if(value & WATCH_RENAME)
		{
		}
		else
		if(value & WATCH_CHANGE)
		{
		}
	}
	else
	if(value & WATCH_TRANSACTION)
	{
	}
	RenderReport();
	Unlock();
}

void ReportWindow::MessageReceived(BMessage *msg)
{
	switch(msg->what)
	{
		case M_PREVIOUS_FIELD:
		{
			if(fStartDateBox->ChildAt(0)->IsFocus())
				fCategoryList->MakeFocus();
			else
			if(fEndDateBox->ChildAt(0)->IsFocus())
				fStartDateBox->MakeFocus();
			break;
		}
		case M_NEXT_FIELD:
		{
			if(fStartDateBox->ChildAt(0)->IsFocus())
				fEndDateBox->MakeFocus();
			else
			if(fEndDateBox->ChildAt(0)->IsFocus())
				fGridView->MakeFocus();
			break;
		}
		case M_REPORT_CASH_FLOW:
		{
			fReportMode = REPORT_CASH_FLOW;
			RenderReport();
			break;
		}
		case M_REPORT_NET_WORTH:
		{
			fReportMode = REPORT_NET_WORTH;
			RenderReport();
			break;
		}
		case M_REPORT_TRANSACTIONS:
		{
			fReportMode = REPORT_TRANSACTIONS;
			RenderReport();
			break;
		}
		case M_REPORT_BUDGET:
		{
			fReportMode = REPORT_BUDGET;
			RenderReport();
			break;
		}
		
		
		
		case M_SUBTOTAL_NONE:
		{
			fSubtotalMode = SUBTOTAL_NONE;
			RenderReport();
			break;
		}
		case M_SUBTOTAL_MONTH:
		{
			fSubtotalMode = SUBTOTAL_MONTH;
			RenderReport();
			break;
		}
		case M_SUBTOTAL_QUARTER:
		{
			fSubtotalMode = SUBTOTAL_QUARTER;
			RenderReport();
			break;
		}
		case M_SUBTOTAL_YEAR:
		{
			fSubtotalMode = SUBTOTAL_YEAR;
			RenderReport();
			break;
		}
		
		case M_START_DATE_CHANGED:
		{
			time_t temp;
			if(gDefaultLocale.StringToDate(fStartDateBox->Text(),temp)==B_OK)
			{
				fStartDate = temp;
				if(fStartDate > fEndDate)
					fStartDate = fEndDate;
				RenderReport();
			}
			else
			{
				ShowAlert(TRANSLATE("Capital Be didn't understand the date you entered."),
						TRANSLATE(
						"Capital Be understands lots of different ways of entering dates. "
						"Apparently, this wasn't one of them. You'll need to change how you "
						"entered this date. Sorry."));
				fStartDateBox->MakeFocus(true);
				break;
			}
			
			BString formatted;
			gDefaultLocale.DateToString(fStartDate,formatted);
			fStartDateBox->SetText(formatted.String());
			
			break;
		}
		case M_END_DATE_CHANGED:
		{
			time_t temp;
			if(gDefaultLocale.StringToDate(fEndDateBox->Text(),temp)==B_OK)
			{
				fEndDate = temp;
				if(fStartDate > fEndDate)
					fStartDate = fEndDate;
				RenderReport();
			}
			else
			{
				ShowAlert(TRANSLATE("Capital Be didn't understand the date you entered."),
						TRANSLATE(
						"Capital Be understands lots of different ways of entering dates. "
						"Apparently, this wasn't one of them. You'll need to change how you "
						"entered this date. Sorry."));
				fEndDateBox->MakeFocus(true);
				break;
			}
			
			BString formatted;
			gDefaultLocale.DateToString(fEndDate,formatted);
			fEndDateBox->SetText(formatted.String());
			break;
		}
		case M_CATEGORIES_CHANGED:
		{
			CalcCategoryString();
			RenderReport();
			break;
		}
		case M_TOGGLE_ACCOUNT:
		{
			RenderReport();
			break;
		}
		case M_TOGGLE_GRAPH:
		{
			if(fGridView->IsHidden())
			{
				fGridView->Show();
				fGraphView->Hide();
			}
			else
			{
				fGridView->Hide();
				fGraphView->Show();
			}
			break;
		}
		default:
			BWindow::MessageReceived(msg);
	}
}

void ReportWindow::FrameResized(float w, float h)
{
	BScrollBar *bar = fCategoryScroller->ScrollBar(B_VERTICAL);
	
	if(fCategoryList->CountItems())
	{
		float itemheight = fCategoryList->ItemAt(0)->Height();
		float itemrange = (fCategoryList->CountItems()+1) * itemheight;
		if(itemrange < fCategoryScroller->Frame().Height())
			bar->SetRange(0,0);
		else
			bar->SetRange(0,itemrange - fCategoryScroller->Frame().Height());
		
		float big, small;
		bar->GetSteps(&small, &big);
		big = (int32)(fCategoryScroller->Frame().Height() * .9);
		bar->SetSteps(small,big);
	}
	else
		bar->SetRange(0,0);
	
	FixGridScrollbar();
}

void ReportWindow::AddAccount(Account *acc)
{
	if(!acc)
		return;
	
	acc->AddObserver(this);
	
	AccountItem *accountitem = new AccountItem(acc);
	fAccountList->AddItem(accountitem);
	
	BString command = "select category from account_";
	command << acc->GetID() << ";";
	CppSQLite3Query query = gDatabase.DBQuery(command.String(),"ReportWindow::AddAccount");
	
	while(!query.eof())
	{
		BString catstr = DeescapeIllegalCharacters(query.getStringField(0));
		if(catstr.CountChars()<1)
		{
			query.nextRow();
			continue;
		}
		
		// Make sure that the category is not already in the list.
		BStringItem *existing = NULL;
		for(int32 k=0; k<fCategoryList->CountItems(); k++)
		{
			BStringItem *item = (BStringItem*)fCategoryList->ItemAt(k);
			if(!item)
				continue;
			
			if(catstr.ICompare(item->Text())==0)
			{
				existing = item;
				break;
			}
		}
		
		if(existing)
		{
			query.nextRow();
			continue;
		}
		
		fCategoryList->AddItem(new BStringItem(catstr.String()));
		
		query.nextRow();
	}
}

void ReportWindow::FixGridScrollbar(void)
{
	BScrollBar *bar = fGridView->ScrollBar(B_VERTICAL);
	BRow *row = fGridView->RowAt(0);
	if(!row)
		return;
	float itemrange = (fGridView->CountRows()+3) * row->Height();;
	if(itemrange < fGridView->Frame().Height() - B_H_SCROLL_BAR_HEIGHT)
		bar->SetRange(0,0);
	else
		bar->SetRange(0,itemrange - fGridView->Frame().Height());

	float big, small;
	bar->GetSteps(&small, &big);
	big = (int32)(fGridView->Frame().Height() * .9);
	bar->SetSteps(small,big);
}

/*
void ReportWindow::CalcAccountString(void)
{
	// Compile list of selected accounts
	fAccountString = "";
	for(int32 i=0; i<gDatabase.CountAccounts(); i++)
	{
		AccountItem *accitem = (AccountItem*)fAccountList->ItemAt(i);
		if(accitem && accitem->IsSelected())
		{
			if(fAccountString.CountChars()>0)
				fAccountString << ",account_" << accitem->account->GetID();
			else
				fAccountString << "account_" << accitem->account->GetID();
		}
	}
}
*/
void ReportWindow::CalcCategoryString(void)
{
	// Compile list of selected categories
	fCategoryString = "";
	for(int32 i=0; i<fCategoryList->CountItems(); i++)
	{
		BStringItem *catitem = (BStringItem*)fCategoryList->ItemAt(i);
		if(catitem && catitem->IsSelected())
		{
			if(fCategoryString.CountChars()>0)
				fCategoryString << ",'" << EscapeIllegalCharacters(catitem->Text()) << "'";
			else
				fCategoryString << "'" << EscapeIllegalCharacters(catitem->Text()) << "'";
		}
	}
}

void ReportWindow::RenderReport(void)
{
	fGridView->Clear();
	BColumn *column = fGridView->ColumnAt(0);
	while(column)
	{
		fGridView->RemoveColumn(column);
		delete column;
		column = fGridView->ColumnAt(0);
	}
	
	if(fAccountList->CountItems()<1 || fAccountList->CurrentSelection()<0)
		return;
	
	switch(fReportMode)
	{
		case REPORT_CASH_FLOW:
		{
			ComputeCashFlow();
			break;
		}
		case REPORT_NET_WORTH:
		{
			ComputeNetWorth();
			break;
		}
		case REPORT_TRANSACTIONS:
		{
			ComputeTransactions();
			break;
		}
		case REPORT_BUDGET:
		{
			ComputeBudget();
			break;
		}
		default:
			break;
	}
	FixGridScrollbar();
}

bool ReportWindow::QuitRequested(void)
{
	// We need to remove this window's observer object from each account's notifier, or else
	// we will crash the app after closing the window and selecting an account
	
	for (int32 i = 0; i < gDatabase.CountAccounts(); i++)
	{
		Account *acc = gDatabase.AccountAt(i);
		if(!acc)
			continue;
		
		acc->RemoveObserver(this);
	}
	gDatabase.RemoveObserver(this);
	return true;
}


int compare_stringitem(const void *item1, const void *item2)
{
	BListItem *listitem1 = *((BListItem**)item1);
	BListItem *listitem2 = *((BListItem**)item2);
	
	BStringItem *stritem1=(BStringItem *)listitem1;
	BStringItem *stritem2=(BStringItem *)listitem2;
	
	int len1 = (stritem1 && stritem1->Text()) ? strlen(stritem1->Text()) : 0;
	int len2 = (stritem2&& stritem2->Text()) ? strlen(stritem2->Text()) : 0;

	if(len1 < 1)
	{
		return (len2 < 1) ? 0 : 1;
	}
	else
	if(len2 < 1)
	{
		return (len1 < 1) ? 0 : -1;
	}
	
	return strcmp(stritem1->Text(),stritem2->Text());
}

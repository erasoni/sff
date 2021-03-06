#include "stdafx.h"
#include "thread.h"
#include "work.h"
#include "FormMain.h"
#include "FileData.h"
#include "helper.h"
#include "dnhelper.h"

#include "../../lsMisc/getStdString.net.h"
#include "../../lsMisc/browseFolder.h"

namespace sff {
	using namespace Ambiesoft;
	using namespace System::Text;
	using namespace System::Windows::Forms;
	using namespace System::Reflection;
	using namespace System::IO;
	using namespace Miszou::ToolManager;

	FormMain::FormMain(System::Collections::Generic::List<String^>^ args)
	{
		args_ = args;
		InitializeComponent();
		// lvProgress->SetStyle(ControlStyles::OptimizedDoubleBuffer, true);

		StringBuilder sb;
		for each(String^ line in args)
		{
			sb.AppendLine(line);
		}
		txtInDir->Text = sb.ToString();

		orgShowErrorText_ = btnShowError->Text;

		HashIni^ ini = Profile::ReadAll(IniPath);
		array<String^>^ as;
		Profile::GetStringArray("recents", "recent", as, ini);
		for each(String^ t in as)
		{
			if (recents_.IndexOf(t) >= 0)
				continue;
			recents_.Add(t);
		}

		AmbLib::LoadFormXYWH(this, "option", ini);

		// Initialize Tools
		// List of Command Macros supported by your application
		List<Macro^>^ macroList = gcnew List<Macro^>();
		macroList->Add(gcnew Macro("${file}", I18N(L"File to open")));

		// filder list
		List<Macro^>^ folderList = gcnew List<Macro^>();

		// Create the ToolManager
		do
		{
			try
			{
				mTools = gcnew Tools(
					ToolsPath,
					macroList,
					folderList,
					gcnew Tools::MacroExpander(this, &FormMain::ExpandToolMacros),
					gcnew System::Windows::Forms::ImageList);
			}
			catch (Exception^ e)
			{
				if (MessageBox::Show(
					I18N(L"Failed to load.") + L" \"" + ToolsPath + L"\"\r\n\r\n" + I18N(L"Reason") + L":\r\n" +
					e->Message + L"\r\n\r\n" + I18N(L"Fix the file and press OK or press Cancel to exit.") + L"\r\n" +
					I18N(L"If this probrem continues, delete the file but you will lost all information about External Tools."),
					Application::ProductName,
					MessageBoxButtons::OKCancel,
					MessageBoxIcon::Error) != System::Windows::Forms::DialogResult::OK)
				{
					exit(-1);
				}
			}
		} while (mTools == nullptr);
	}
	String^ FormMain::ExpandToolMacros(String^ str)
	{
		if (String::IsNullOrEmpty(str))
			return str;
		if (lvProgress->SelectedItems->Count == 0)
			return String::Empty;
		str = str->Replace("${file}", lvProgress->SelectedItems[0]->Text);
		return str;
	}
	void FormMain::BuildExternalToolMenu(ToolStripMenuItem^ item, int startindex)
	{
		// extMacroUrl_ = extMacroActiveTabUrl_ = extMacroSelectedTabUrl_ = String::Empty;
		mTools->BuildToolMenu(item, startindex);

		if (item->DropDownItems->Count == 0)
		{
			ToolStripMenuItem^ subitem = gcnew ToolStripMenuItem();
			subitem->Text = I18N(L"<No External Tools registered>");
			subitem->Enabled = false;
			item->DropDownItems->Add(subitem);
		}

		// add menu for launch the dialog of external tools
		item->DropDownItems->Add(gcnew ToolStripSeparator());

		ToolStripMenuItem^ externalToolsItem = gcnew ToolStripMenuItem();
		externalToolsItem->Text = tsmiExternalTools->Text;
		externalToolsItem->Click += gcnew System::EventHandler(this, &FormMain::tsmiExternalTools_Click);
		item->DropDownItems->Add(externalToolsItem);

	}
	public ref class ListViewItemComparer : System::Collections::IComparer
	{
	private:
		static int rev_;
		int _column;

	public:
		static ListViewItemComparer()
		{
			rev_ = 1;
		}
		ListViewItemComparer(int col)
		{
			_column = col;
			rev_ = rev_ * -1;
		}

		virtual int Compare(Object^ o1, Object^ o2)
		{
			//ListViewItem�̎擾
			ListViewItem^ item1 = (ListViewItem^)o1;
			ListViewItem^ item2 = (ListViewItem^)o2;

			ULL u1;
			ULL u2;
			System::UInt64::TryParse(item1->SubItems[_column]->Text, u1);
			System::UInt64::TryParse(item2->SubItems[_column]->Text, u2);

			LONGLONG uret = u1 - u2;
			int ret = 0;
			if (uret == 0)
				ret = 0;
			else if (uret > 0)
				ret = 1;
			else
				ret = -1;

			return ret * rev_;
		}
	};

	void FormMain::SetTitle(String^ addition)
	{
		if (String::IsNullOrEmpty(addition))
		{
			this->Text = Application::ProductName;
		}
		else
		{
			this->Text = addition + " - " + Application::ProductName;
		}
	}
	System::Void FormMain::FormMain_Load(System::Object^  sender, System::EventArgs^  e)
	{
		ThreadOn(false);
		cmbNameReg->Items->Add(L"\\.pdf$");
		cmbNameReg->SelectedIndex = 0;
		cmbNameReg->Text = L"";

		cmbMinSize->Items->Add(L"10M");
		cmbMinSize->SelectedIndex = 0;
		cmbMinSize->Text = L"";

		SetTitle(nullptr);
		Application::Idle += gcnew EventHandler(this, &FormMain::onIdle);

		// lblVersion->Text = "SFF ver" + System::Reflection::Assembly::GetExecutingAssembly()->GetName()->Version;
		lblVersion->Text = "SFF ver " + AmbLib::getAssemblyVersion(Assembly::GetExecutingAssembly(),3);

		tpSettings_Resize(this, nullptr);
	}
	System::Void FormMain::tpSettings_Resize(System::Object^ sender, System::EventArgs^ e)
	{
		int marginX = 8;
		int marginY = 4;
		System::Drawing::Size size(
			btnAnchor->Location.X - lblFilter->Location.X - marginX,
			btnAnchor->Location.Y - btnAdd->Location.Y -marginY
		);
		txtInDir->Size = size;
	}
	System::Void FormMain::onIdle(System::Object^, System::EventArgs^)
	{
		slItemCount->Text = I18N(L"Items : ") + lvProgress->Items->Count.ToString();
		slGroupCount->Text = I18N(L"Groups : ") + lvProgress->Groups->Count.ToString();

		bool on = (gcurthread != NULL);

		btnPause->Enabled = on && !bSuspended_;
		btnResume->Enabled = on && bSuspended_;
	}

	System::Void FormMain::lvProgress_ColumnClick(System::Object^  sender, System::Windows::Forms::ColumnClickEventArgs^  e)
	{
		lvProgress->ListViewItemSorter =
			gcnew ListViewItemComparer(e->Column);
		lvProgress->Sort();
	}
	System::Void FormMain::btnStart_Click(System::Object^  sender, System::EventArgs^  e)
	{
		if (gcurthread == NULL)
		{
			// List<String^>^ inlines = gcnew List<String^>;
			TSTRINGVECTOR vinlines;
			String^ title = String::Empty;
			for each(String^ inl in txtInDir->Lines)
			{
				inl = inl->TrimStart();
				if (String::IsNullOrEmpty(inl))
					continue;

				try
				{
					System::IO::DirectoryInfo di(inl);
					if (!di.Exists)
					{
						if (System::Windows::Forms::DialogResult::Yes != MessageBox::Show(L"\"" + inl + L"\"" + L" does not exist.\r\nContinue?",
							Application::ProductName,
							MessageBoxButtons::YesNo,
							MessageBoxIcon::Question,
							MessageBoxDefaultButton::Button2))
						{
							return;
						}
					}
				}
				catch (System::Exception^ ex)
				{
					CppUtils::Alert(ex->Message + L"\"" + inl + L"\"");
					return;
				}
				// inlines->Add(inl);
				vinlines.push_back(getStdWstring(inl->TrimEnd(L'\\')));
				title += inl + "|";
			}
			title = title->TrimEnd('|');

			if (vinlines.size() == 0)
			{
				CppUtils::Alert(L"No Folders specified.");
				return;
			}

			// save recents
			{
				int insertPos = 0;
				for each(const tstring& t in vinlines)
				{
					String^ dir = gcnew String(t.c_str());
					if (String::IsNullOrEmpty(dir))
						continue;
					int nFound = recents_.IndexOf(dir);
					if (nFound >= 0)
						recents_.RemoveAt(nFound);

					recents_.Insert(insertPos++, dir);
				}
			}

			if (vinlines.size() < 2 && chkEachFolder->Checked)
			{
				CppUtils::Alert(L"\"Find from each line\" is invalid when only one entry exists.");
				return;

			}

			String^ namereg = cmbNameReg->Text;
			try
			{
				gcnew System::Text::RegularExpressions::Regex(namereg);
			}
			catch (System::Exception^ ex)
			{
				CppUtils::Alert(ex->Message);
				return;
			}

			ULL uminsize = 0;
			do
			{
				String^ text = cmbMinSize->Text;
				if (String::IsNullOrEmpty(text))
					break;

				Char lc = text[text->Length - 1];
				ULL mul = 1;
				if (L'0' <= lc && lc <= L'9')
				{

				}
				else
				{
					text = text->Substring(0, text->Length - 1);


					switch (lc)
					{
					case L'k':mul = 1000;   break;
					case L'K':mul = 1024;   break;
					case L'm':mul = 1000 * 1000;   break;
					case L'M':mul = 1024 * 1024;   break;
					case L'g':mul = 1000 * 1000 * 1000;   break;
					case L'G':mul = 1024 * 1024 * 1024;   break;
					default:
					{
						MessageBox::Show(L"File size illegal.");
						return;
					}
					break;
					}
				}

				if (!System::UInt64::TryParse(text, uminsize))
				{
					MessageBox::Show(L"File size illegal.");
					return;
				}

				uminsize = uminsize * mul;
			} while (false);


			// checkdone


			lvProgress->Items->Clear();
			lvProgress->Groups->Clear();
			groupI_.Clear();
			frmError.lvError->Items->Clear();

			//LVDATA^ lvdata = (LVDATA^)lvProgress->Tag;
			//if(!lvdata)
			//{
			//	lvdata=gcnew LVDATA;
			//	lvProgress->Tag=lvdata;
			//}
			//lvdata->Clear();

			//LPCTSTR pDir = _tcsdup(_T("E:\\T\\10"));
			pin_ptr<const wchar_t> pWC = PtrToStringChars(namereg);
			THREADPASSDATA* pData = new THREADPASSDATA(
				0,
				NULL,
				(HWND)this->Handle.ToPointer(),
				0,
				&vinlines,
				pWC,
				uminsize,
				chkEachFolder->Checked);
			uintptr_t threadhandle = _beginthreadex(NULL, 0, startOfSearch, (void*)pData, CREATE_SUSPENDED, NULL);
			if (threadhandle == 0)
			{
				MessageBox::Show(L"thread creation failed");
				return;
			}
			gthid++;
			gcurthread = (HANDLE)threadhandle;
			pData->thid_ = gthid;
			pData->thisthread_ = (HANDLE)threadhandle;
			DVERIFY_NOT(ResumeThread((HANDLE)threadhandle), 0xFFFFFFFF);

			ThreadOn(true);
			SetTitle(title);
			tabMain->SelectedTab = tbProgress;

		}
		else
		{
			try
			{
				SuspendThread(gcurthread); // (gcurthread,THREAD_PRIORITY_LOWEST);
				if (System::Windows::Forms::DialogResult::Yes !=
					CppUtils::YesOrNo(this, I18N(L"Are you sure you want to cancel?"), MessageBoxDefaultButton::Button1))
				{
					return;
				}
			}
			finally
			{
				ResumeThread(gcurthread);// SetThreadPriority(gcurthread,THREAD_PRIORITY_NORMAL);
			}
			CloseThread();

			ThreadOn(false);
		}
	}

	void FormMain::ThreadOn(bool on)
	{
		btnStart->Text = on ? L"&Cancel" : L"&Start";
	}


	System::Void FormMain::btnPause_Click(System::Object^  sender, System::EventArgs^  e)
	{
		SuspendThread(gcurthread);
		bSuspended_ = true;
	}
	System::Void FormMain::btnResume_Click(System::Object^  sender, System::EventArgs^  e)
	{
		while (ResumeThread(gcurthread) > 0)
			;
		bSuspended_ = false;
	}


	void FormMain::CloseThread()
	{
		if (gcurthread == NULL)
			return;

		Enabled = false;
		Application::DoEvents();

		gthid++;
		while (ResumeThread(gcurthread) > 0)
			;

		int waited = 0;
		while (gcurthread != NULL && WAIT_TIMEOUT == WaitForSingleObject(gcurthread, 500))
		{
			Application::DoEvents();
			//if(waited++ > 10)
			//{
			//	TerminateThread(gcurthread, -1);
			//	break;
			//}
		}
		gcurthread = NULL;
		Enabled = true;
		DTRACE(L"Closing Done.");
	}

	bool FormMain::IsThreadWorking::get()
	{
		return gcurthread != NULL;
	}

	System::Void FormMain::FormMain_FormClosing(System::Object^  sender, System::Windows::Forms::FormClosingEventArgs^  e)
	{
		if (IsThreadWorking)
		{
			if (System::Windows::Forms::DialogResult::Yes != CppUtils::YesOrNo(I18N("Are you sure you want to close the application?")))
			{
				e->Cancel = true;
				return;
			}
		}

		e->Cancel = false;

		// save
		bool bSaveOK = true;
		HashIni^ ini = Profile::ReadAll(IniPath);
		bSaveOK &= Profile::WriteStringArray("recents", "recent", recents_.ToArray(), ini);
		bSaveOK &= AmbLib::SaveFormXYWH(this, "option", ini);
		if (bSaveOK)
			bSaveOK &= Profile::WriteAll(ini, IniPath);
		if (!bSaveOK)
		{
			CppUtils::Alert(I18N(L"Failed to save ini."));
		}

		CloseThread();
		Application::Idle -= gcnew EventHandler(this, &FormMain::onIdle);
	}

	void FormMain::AddtoDirs(... array<String^>^ folders)
	{
		for each(String^ folder in folders)
		{
			if (!txtInDir->Text->EndsWith(L"\n") && !String::IsNullOrEmpty(txtInDir->Text))
				txtInDir->Text += L"\r\n";
			txtInDir->Text += folder + L"\r\n";
		}
	}
	System::Void FormMain::btnAdd_Click(System::Object^  sender, System::EventArgs^  e)
	{
		String^ folder;
		browseFolder(this, Application::ProductName, folder);

		AddtoDirs(folder);
	}
	void FormMain::OnRecentItemClick(System::Object ^sender, System::EventArgs ^e)
	{
		ToolStripMenuItem^ tsi = (ToolStripMenuItem^)sender;

		AddtoDirs(tsi->Tag->ToString());
	}

	System::Void FormMain::btnRecents_Click(System::Object^  sender, System::EventArgs^  e)
	{
		ctxRecents_.Items->Clear();

		for each(String^ recent in recents_)
		{
			ToolStripMenuItem^ tsi = gcnew ToolStripMenuItem();
			tsi->Text = recent;
			tsi->Tag = recent;
			tsi->Click += gcnew System::EventHandler(this, &sff::FormMain::OnRecentItemClick);
			ctxRecents_.Items->Add(tsi);
		}

		//ctxRecents_.Show();// btnRecents->PointToScreen(btnRecents->Location));
		System::Drawing::Point pt(btnRecents->Parent->PointToScreen(btnRecents->Location));
		pt.X += btnRecents->Size.Width;
		ctxRecents_.Show(pt);
	}

	System::Void FormMain::explorerToolStripMenuItem_Click(System::Object^  sender, System::EventArgs^  e)
	{
		try
		{
			pin_ptr<const wchar_t> pIn = PtrToStringChars(lvProgress->SelectedItems[0]->Text);

			tstring arg;
			arg += _T("/select,\"");
			arg += pIn;
			arg += _T("\",/n");
#pragma comment(lib, "shell32.lib")
			ShellExecute((HWND)this->Handle.ToPointer(), _T("open"), _T("explorer.exe"),
				arg.c_str(), NULL, SW_SHOW);
		}
		catch (System::Exception^)
		{
			// ExceptionMessageBox(ex);
		}
		finally
		{

		}
	}

	System::Void FormMain::btnShowError_Click(System::Object^  sender, System::EventArgs^  e)
	{
		if (!frmError.Visible)
			frmError.Show();

		frmError.BringToFront();
	}

	System::Void FormMain::tsbRemoveNonExistFiles_Click(System::Object^  sender, System::EventArgs^  e)
	{
		for each(ListViewItem^ item in this->lvProgress->Items)
		{
			if (!System::IO::File::Exists(item->Text))
				lvProgress->Items->Remove(item);
		}
	}

	System::Void FormMain::linkHomepage_LinkClicked(System::Object^  sender, System::Windows::Forms::LinkLabelLinkClickedEventArgs^  e)
	{
		AmbLib::OpenUrlWithBrowser("https://ambiesoft.github.io/sff/");
	}

	System::Void FormMain::tsmiExit_Click(System::Object^ sender, System::EventArgs^ e)
	{
		Close();
	}
	System::Void FormMain::cmExternalTools_DropDownOpening(System::Object^ sender, System::EventArgs^ e)
	{
		cmExternalTools->DropDownItems->Clear();
		BuildExternalToolMenu(cmExternalTools, 0);
	}
	System::Void FormMain::tsmiExternalTools_Click(System::Object^ sender, System::EventArgs^ e)
	{
		// We need try-catch because m_Tools_->Edit writes to file.
		try
		{
			if (System::Windows::Forms::DialogResult::OK !=
				mTools->Edit(
					Miszou::ToolManager::Tools::EditFlags::AllowLockedUIEdit)
				)
			{
				return;
			}
		}
		catch (Exception^ ex)
		{
			CppUtils::Alert(ex->Message);
			return;
		}
	}

}


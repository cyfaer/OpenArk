/****************************************************************************
**
** Copyright (C) 2019 BlackINT3
** Contact: https://github.com/BlackINT3/OpenArk
**
** GNU Lesser General Public License Usage (LGPL)
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file. Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
****************************************************************************/
#include <WinSock2.h>

//https://social.msdn.microsoft.com/Forums/windowsdesktop/en-US/8fd93a3d-a794-4233-9ff7-09b89eed6b1f/compiling-with-wfp?forum=wfp
#include "include/fwpmu.h"
#include "network.h"
#pragma comment(lib, "fwpuclnt.lib")
#pragma comment(lib, "Rpcrt4.lib")


BOOLEAN GuidEqual(_In_ const GUID* pGUIDAlpha, _In_ const GUID* pGUIDOmega)
{
	RPC_STATUS status = RPC_S_OK;
	UINT32     areEqual = FALSE;

	do
	{
		if (pGUIDAlpha == 0 ||
			pGUIDOmega == 0)
		{
			if ((pGUIDAlpha == 0 &&
				pGUIDOmega) ||
				(pGUIDAlpha &&
					pGUIDOmega == 0))
				break;
		}

		if (pGUIDAlpha == 0 &&
			pGUIDOmega == 0)
		{
			areEqual = TRUE;
			break;
		}

		areEqual = UuidEqual((UUID*)pGUIDAlpha,
			(UUID*)pGUIDOmega,
			&status);

	} while (false);

	return (BOOLEAN)areEqual;
}

bool EnumWfpCallouts(std::vector<CALLOUT_INFO>& CalloutIDs)
{
	bool Result = false;
	HANDLE EngineHandle = NULL;
	UINT32       status = NO_ERROR;
	FWPM_SESSION session = { 0 };
	HANDLE EnumHandle = NULL;
	FWPM_CALLOUT_ENUM_TEMPLATE* pCalloutEnumTemplate = NULL;
	session.displayData.name = L"WFPSampler's User Mode Session";
	session.flags = 0;

	do
	{
		status = FwpmEngineOpen0(0,
			RPC_C_AUTHN_WINNT,
			0,
			&session,
			&EngineHandle);
		if (status != NO_ERROR) {
			break;
		}

		status = FwpmCalloutCreateEnumHandle(EngineHandle,
			pCalloutEnumTemplate,
			&EnumHandle);
		if (status != NO_ERROR) {
			break;
		}

		UINT32 NumEntries = 0;
		FWPM_CALLOUT** ppCallouts = 0;
		status = FwpmCalloutEnum0(EngineHandle,
			EnumHandle,
			0xFFFFFFFF,
			&ppCallouts,
			&NumEntries);
		if (status != NO_ERROR) {
			break;
		}

		if (ppCallouts)
		{
			for (DWORD Index = 0; Index < NumEntries; Index++)
			{
				CALLOUT_INFO CalloutInfo;
				CalloutInfo.CalloutId = ppCallouts[Index]->calloutId;
				RtlCopyMemory(&CalloutInfo.CalloutKey, &ppCallouts[Index]->calloutKey, sizeof(GUID));
				CalloutIDs.push_back(CalloutInfo);
			}
			Result = true;
		}

	} while (false);

	if (EnumHandle) {
		FwpmCalloutDestroyEnumHandle(EngineHandle, EnumHandle);
	}

	if (EngineHandle) {
		status = FwpmEngineClose(EngineHandle);
	}

	if (pCalloutEnumTemplate) {
		delete pCalloutEnumTemplate;
		pCalloutEnumTemplate = NULL;
	}

	return Result;
}

UINT64 GetFilterIDByCalloutKey(const GUID* CalloutKey)
{
	UINT64 Result = 0;
	HANDLE EngineHandle = NULL;
	LONG  Status = NO_ERROR;
	FWPM_SESSION Session = { 0 };
	HANDLE EnumHandle = 0;
	FWPM_FILTER_ENUM_TEMPLATE* pFilterEnumTemplate = NULL;
	Session.displayData.name = L"WFPSampler's User Mode Session";
	Session.flags = 0;

	do
	{
		Status = FwpmEngineOpen(0,
			RPC_C_AUTHN_WINNT,
			0,
			&Session,
			&EngineHandle);
		if (Status != NO_ERROR)
		{
			break;
		}

		UINT32                               NumEntries = 0;
		FWPM_FILTER**                        ppFilters = 0;
		Status = FwpmFilterCreateEnumHandle0(EngineHandle,
			pFilterEnumTemplate,
			&EnumHandle);
		if (Status != NO_ERROR)
		{
			break;
		}

		Status = FwpmFilterEnum0(EngineHandle,
			EnumHandle,
			0xFFFFFFFF,
			&ppFilters,
			&NumEntries);
		if (Status != NO_ERROR)
		{
			break;
		}

		if (Status == NO_ERROR && ppFilters && NumEntries)
		{
			for (UINT32 Index = 0; Index < NumEntries; Index++)
			{
				if (GuidEqual(&ppFilters[Index]->action.calloutKey,
					CalloutKey))
				{
					Result = ppFilters[Index]->filterId;
					break;
				}
			}
		}

	} while (false);

	if (EngineHandle) {
		Status = FwpmEngineClose(EngineHandle);
	}

	if (pFilterEnumTemplate) {
		delete pFilterEnumTemplate;
		pFilterEnumTemplate = NULL;
	}

	return Result;
}

bool DeleteFilterById(UINT64 FilterId)
{
	bool Result = false;
	HANDLE EngineHandle = NULL;
	LONG       Status = NO_ERROR;
	FWPM_SESSION Session = { 0 };
	Session.displayData.name = L"WFPSampler's User Mode Session";
	Session.flags = 0;

	do
	{
		Status = FwpmEngineOpen(0,
			RPC_C_AUTHN_WINNT,
			0,
			&Session,
			&EngineHandle);
		if (Status != NO_ERROR) {
			break;
		}

		Status = FwpmFilterDeleteById(EngineHandle, FilterId);
		if (Status != NO_ERROR) {
			break;
		}

		Result = true;
	} while (false);

	if (EngineHandle) {
		Status = FwpmEngineClose(EngineHandle);
	}

	return Result;
}

bool WfpSortFilterProxyModel::lessThan(const QModelIndex &left, const QModelIndex &right) const {
	auto s1 = sourceModel()->data(left); auto s2 = sourceModel()->data(right);
	return QString::compare(s1.toString(), s2.toString(), Qt::CaseInsensitive) < 0;
}

KernelNetwork::KernelNetwork()
{

}

KernelNetwork::~KernelNetwork()
{

}

void KernelNetwork::onTabChanged(int index)
{
	switch (index) {
	case 0:
		ShowWfpInfo();
		break;
	default:
		break;
	}
	CommonTabObject::onTabChanged(index);
}

bool KernelNetwork::eventFilter(QObject *obj, QEvent *e)
{
	if (e->type() == QEvent::ContextMenu) {
		QMenu *menu = nullptr;
		if (obj == ui_->hostsFileListWidget) menu = hosts_menu_;
		QContextMenuEvent *ctxevt = dynamic_cast<QContextMenuEvent*>(e);
		if (ctxevt && menu) {
			menu->move(ctxevt->globalPos());
			menu->show();
		}
	}
	if (e->type() == QEvent::KeyPress) {
		QKeyEvent *keyevt = dynamic_cast<QKeyEvent*>(e);
		if (keyevt->matches(QKeySequence::Delete)) {
			for (auto &action : hosts_menu_->actions()) {
				if (action->text() == "Delete") emit action->trigger();
			}
		}
	}
	return QWidget::eventFilter(obj, e);
}

void KernelNetwork::ModuleInit(Ui::Kernel *ui, Kernel *kernel)
{
	this->ui_ = ui;
	this->kernel_ = kernel;
	Init(ui->tabNetwork, TAB_KERNEL, TAB_KERNEL_NETWORK);

	InitWfpView();
	InitHostsView();
}

void KernelNetwork::InitWfpView()
{
	wfp_model_ = new QStandardItemModel;
	QTreeView *view = ui_->wfpView;
	proxy_wfp_ = new WfpSortFilterProxyModel(view);
	proxy_wfp_->setSourceModel(wfp_model_);
	proxy_wfp_->setDynamicSortFilter(true);
	proxy_wfp_->setFilterKeyColumn(1);
	view->setModel(proxy_wfp_);
	view->selectionModel()->setModel(proxy_wfp_);
	view->header()->setSortIndicator(-1, Qt::AscendingOrder);
	view->setSortingEnabled(true);
	view->viewport()->installEventFilter(kernel_);
	view->installEventFilter(kernel_);
	std::pair<int, QString> colum_layout[] = {
		{ 130, tr("ID") },
		{ 100, tr("Key") },
		{ 200, tr("Name") },
	};
	QStringList name_list;
	for (auto p : colum_layout) {
		name_list << p.second;
	}
	wfp_model_->setHorizontalHeaderLabels(name_list);
	for (int i = 0; i < _countof(colum_layout); i++) {
		view->setColumnWidth(i, colum_layout[i].first);
	}
	view->setEditTriggers(QAbstractItemView::NoEditTriggers);
}

void KernelNetwork::InitHostsView()
{
	hosts_dir_ = UNONE::OsSystem32DirW() + L"\\drivers\\etc";
	hosts_file_ = hosts_dir_ + L"\\hosts";

	auto GetCurrentHostsName = [=]()->std::wstring {
		std::wstring hosts;
		auto cur = ui_->hostsFileListWidget->currentItem();
		if (cur) {
			hosts = cur->text().toStdWString();
		}
		return std::move(hosts);
	};

	auto GetCurrentHostsPath = [=]()->std::wstring {
		std::wstring hosts = GetCurrentHostsName();
		if (!hosts.empty()) hosts = hosts_dir_ + L"\\" + hosts;
		return std::move(hosts);
	};

	auto ReloadHostsData = [=]() {
		std::string data;
		auto &&hosts = GetCurrentHostsPath();
		UNONE::FsReadFileDataW(hosts, data);
		ui_->hostsDataEdit->setText(StrToQ(data));
	};

	auto WriteHostsData = [=](std::wstring path = L"") {
		std::string data = ui_->hostsDataEdit->toPlainText().toStdString();
		std::wstring hosts;
		if (path.empty()) hosts = GetCurrentHostsPath();
		else hosts = path;
		UNONE::StrReplaceA(data, "\n", "\r\n");
		UNONE::FsWriteFileDataW(hosts, data);
	};

	auto ReloadHostsList = [=]() {
		auto row = ui_->hostsFileListWidget->currentRow();
		ui_->hostsFileListWidget->clear();
		std::vector<std::wstring> names;
		UNONE::DirEnumCallbackW fcb = [&](wchar_t* path, wchar_t* name, void* param)->bool {
			if (UNONE::FsIsDirW(path)) return true;
			size_t yy=UNONE::StrIndexIW(std::wstring(name), std::wstring(L"hosts"));
			if (UNONE::StrIndexIW(std::wstring(name), std::wstring(L"hosts")) != 0) return true;
			names.push_back(name);
			return true;
		};
		UNONE::FsEnumDirectoryW(hosts_dir_, fcb);
		for (auto &n : names) {
			ui_->hostsFileListWidget->addItem(WStrToQ(n));
		}
		ui_->hostsFileListWidget->setCurrentRow(row);
	};

	connect(ui_->hostsFileListWidget, &QListWidget::itemSelectionChanged, [=] {
		ReloadHostsData();
	});

	connect(ui_->hostsReloadBtn, &QPushButton::clicked, [=] {
		ReloadHostsData();
		ReloadHostsList();
	});

	connect(ui_->hostsSaveBtn, &QPushButton::clicked, [=] {
		WriteHostsData();
	});

	connect(ui_->hostsBackupBtn, &QPushButton::clicked, [=] {
		bool ok;
		SYSTEMTIME systime;
		GetSystemTime(&systime);
		QString def = WStrToQ(UNONE::TmFormatSystemTimeW(systime, L"YMD-HWS"));
		QString text = QInputDialog::getText(this, tr("Hosts Backup"), tr("Please input file name: (hosts-***)"), QLineEdit::Normal, def, &ok);
		if (ok && !text.isEmpty()) {
			auto &&hosts = hosts_dir_ + L"\\hosts-" + text.toStdWString();
			WriteHostsData(hosts);
			ReloadHostsList();
		}
	});

	connect(ui_->hostsClearBtn, &QPushButton::clicked, [=] {
		ui_->hostsDataEdit->clear();
	});

	connect(ui_->hostsDirBtn, &QPushButton::clicked, [&] {
		ShellRun(WStrToQ(hosts_dir_), "");
	});

	if (!UNONE::FsIsExistedW(hosts_file_)) UNONE::FsWriteFileDataW(hosts_file_, "# 127.0.0.1 localhost\n# ::1 localhost");		
	ReloadHostsList();
	ui_->hostsFileListWidget->setCurrentRow(0);

	ui_->hostsFileListWidget->installEventFilter(this);
	hosts_menu_ = new QMenu();

	hosts_menu_->addAction(tr("Rename"), kernel_, [=] {
		bool ok;
		std::wstring &&old = GetCurrentHostsPath();
		auto && name = UNONE::FsPathToNameW(old);
		UNONE::StrReplaceIW(name, L"hosts-");
		QString text = QInputDialog::getText(this, tr("Hosts Rename"), tr("Please input file name: (hosts-***)"), QLineEdit::Normal, WStrToQ(name), &ok);
		if (ok) {
			DeleteFileW(old.c_str());
			std::wstring hosts;
			if (!text.isEmpty()) {
				hosts = hosts_dir_ + L"\\hosts-" + text.toStdWString();
			} else {
				hosts = hosts_dir_ + L"\\hosts";
			}
			WriteHostsData(hosts);
			ReloadHostsList();
		}
	});
	hosts_menu_->addAction(tr("Backup"), kernel_, [=] {
		emit ui_->hostsBackupBtn->click();
	});
	hosts_menu_->addAction(tr("Reload"), kernel_, [=] {
		emit ui_->hostsReloadBtn->click();
	});
	auto copy_menu = new QMenu();
	copy_menu->addAction(tr("File Name"))->setData(0);
	copy_menu->addAction(tr("File Path"))->setData(1);
	copy_menu->setTitle(tr("Copy"));
	connect(copy_menu, &QMenu::triggered, [=](QAction* action) {
		auto idx = action->data().toInt();
		std::wstring data;
		switch (idx) {
		case 0: data = GetCurrentHostsName(); break;
		case 1: data = GetCurrentHostsPath(); break;
		}
		ClipboardCopyData(UNONE::StrToA(data));
	});

	hosts_menu_->addAction(copy_menu->menuAction());
	hosts_menu_->addSeparator();
	hosts_menu_->addAction(tr("Delete"), kernel_, [=] {
		DeleteFileW(GetCurrentHostsPath().c_str());
		emit ui_->hostsReloadBtn->click();
	}, QKeySequence::Delete);
	hosts_menu_->addAction(tr("Delete All"), kernel_, [=] {
		if (QMessageBox::warning(this, tr("Warning"), tr("Are you sure to delete all hosts file(include backups)?"),
			QMessageBox::Yes | QMessageBox::No) == QMessageBox::No) {
			return;
		}
		for (int i = 0; i < ui_->hostsFileListWidget->count(); i++) {
			auto name = ui_->hostsFileListWidget->item(i)->text();
			auto path = hosts_dir_ + L"\\" + QToWStr(name);
			DeleteFileW(path.c_str());
		}
		emit ui_->hostsReloadBtn->click();
	});
}

void KernelNetwork::ShowWfpInfo()
{
	DISABLE_RECOVER();
	ClearItemModelData(wfp_model_, 0);

	std::vector<CALLOUT_INFO> infos;
	EnumWfpCallouts(infos);

	for (auto item : infos) {
		auto id_item = new QStandardItem(DWordToHexQ(item.CalloutId));
		auto key_item = new QStandardItem(DWordToHexQ(item.CalloutKey));
		auto name_item = new QStandardItem(item.ModuleName);
		auto count = wfp_model_->rowCount();
		wfp_model_->setItem(count, 0, id_item);
		wfp_model_->setItem(count, 1, key_item);
		wfp_model_->setItem(count, 2, name_item);
	}
}
// simplewall
// Copyright (c) 2016-2024 Henry++

#include "global.h"

VOID _app_getapptooltipstring (
	_Inout_ PR_STRINGBUILDER buffer,
	_In_ ULONG_PTR app_hash,
	_In_opt_ PITEM_NETWORK ptr_network,
	_In_opt_ PITEM_LOG ptr_log
)
{
	PITEM_APP_INFO ptr_app_info;
	PITEM_APP ptr_app;
	PR_STRING string;
	PR_STRING value;
	PR_STRING path = NULL;
	R_STRINGBUILDER sb;

	ptr_app = _app_getappitem (app_hash);

	// file path
	if (ptr_app)
	{
		if (ptr_app->real_path)
		{
			path = ptr_app->real_path;
		}
		else if (ptr_app->display_name)
		{
			path = ptr_app->display_name;
		}
		else
		{
			path = ptr_app->original_path;
		}
	}
	else if (ptr_network)
	{
		path = ptr_network->path;
	}
	else if (ptr_log)
	{
		if (ptr_log->path)
			path = ptr_log->path;
	}

	if (path)
	{
		_r_obj_appendstringbuilder2 (buffer, &path->sr);

		_r_obj_appendstringbuilder (buffer, SZ_CRLF);
	}

	// file information
	_r_obj_initializestringbuilder (&sb, 512);

	// file display name
	if (ptr_app)
	{
		if (ptr_app->display_name)
		{
			_r_obj_appendstringbuilder (&sb, SZ_TAB);
			_r_obj_appendstringbuilder2 (&sb, &ptr_app->display_name->sr);
			_r_obj_appendstringbuilder (&sb, SZ_CRLF);
		}
	}

	// file version
	ptr_app_info = _app_getappinfobyhash2 (app_hash);

	if (ptr_app_info)
	{
		if (!_r_obj_isstringempty (ptr_app_info->version_info))
		{
			_r_obj_appendstringbuilder (&sb, SZ_TAB);
			_r_obj_appendstringbuilder2 (&sb, &ptr_app_info->version_info->sr);
			_r_obj_appendstringbuilder (&sb, SZ_CRLF);
		}
	}

	// compile
	if (!_r_obj_isstringempty2 (sb.string))
	{
		string = _r_obj_concatstrings (
			2,
			_r_locale_getstring (IDS_FILE),
			L":\r\n"
		);

		_r_obj_insertstringbuilder2 (&sb, 0, string);
		_r_obj_appendstringbuilder2 (buffer, &sb.string->sr);

		_r_obj_dereference (string);
	}

	// comment
	if (ptr_app)
	{
		if (!_r_obj_isstringempty (ptr_app->comment))
		{
			string = _r_obj_concatstrings (
				2,
				_r_locale_getstring (IDS_COMMENT),
				L":\r\n"
			);

			_r_obj_appendstringbuilder2 (buffer, &string->sr);

			_r_obj_appendstringbuilder (buffer, SZ_TAB);
			_r_obj_appendstringbuilder2 (buffer, &ptr_app->comment->sr);
			_r_obj_appendstringbuilder (buffer, SZ_CRLF);

			_r_obj_dereference (string);
		}
	}

	// file signature
	if (ptr_app_info)
	{
		if (!_r_obj_isstringempty (ptr_app_info->signature_info))
		{
			string = _r_obj_concatstrings (
				4,
				_r_locale_getstring (IDS_SIGNATURE),
				L":\r\n" SZ_TAB,
				ptr_app_info->signature_info->buffer,
				SZ_CRLF
			);

			_r_obj_appendstringbuilder2 (buffer, &string->sr);

			_r_obj_dereference (string);
		}
	}

	_r_obj_deletestringbuilder (&sb);

	// app timer
	if (ptr_app)
	{
		if (_app_istimerset (ptr_app))
		{
			value = _r_format_interval (ptr_app->timer - _r_unixtime_now (), TRUE);

			if (value)
			{
				string = _r_obj_concatstrings (
					4,
					_r_locale_getstring (IDS_TIMELEFT),
					L":" SZ_TAB_CRLF,
					value->buffer,
					SZ_CRLF
				);

				_r_obj_appendstringbuilder2 (buffer, &string->sr);

				_r_obj_dereference (string);
				_r_obj_dereference (value);
			}
		}
	}

	// app rules
	value = _app_appexpandrules (app_hash, SZ_TAB_CRLF);

	if (value)
	{
		string = _r_obj_concatstrings (
			4,
			_r_locale_getstring (IDS_RULE),
			L":" SZ_TAB_CRLF,
			value->buffer,
			SZ_CRLF
		);

		_r_obj_appendstringbuilder2 (buffer, &string->sr);

		_r_obj_dereference (string);
		_r_obj_dereference (value);
	}

	// app notes
	if (ptr_app)
	{
		_r_obj_initializestringbuilder (&sb, 512);

		// app type
		if (ptr_app->type == DATA_APP_NETWORK)
		{
			_r_obj_appendstringbuilder (&sb, SZ_TAB);
			_r_obj_appendstringbuilder (&sb, _r_locale_getstring (IDS_HIGHLIGHT_NETWORK));
			_r_obj_appendstringbuilder (&sb, SZ_CRLF);
		}
		else if (ptr_app->type == DATA_APP_PICO)
		{
			_r_obj_appendstringbuilder (&sb, SZ_TAB);
			_r_obj_appendstringbuilder (&sb, _r_locale_getstring (IDS_HIGHLIGHT_PICO));
			_r_obj_appendstringbuilder (&sb, SZ_CRLF);
		}

		// app settings
		if (_app_isappfromsystem (ptr_app->real_path, app_hash))
		{
			_r_obj_appendstringbuilder (&sb, SZ_TAB);
			_r_obj_appendstringbuilder (&sb, _r_locale_getstring (IDS_HIGHLIGHT_SYSTEM));
			_r_obj_appendstringbuilder (&sb, SZ_CRLF);
		}

		if (_app_network_isapphaveconnection (app_hash))
		{
			_r_obj_appendstringbuilder (&sb, SZ_TAB);
			_r_obj_appendstringbuilder (&sb, _r_locale_getstring (IDS_HIGHLIGHT_CONNECTION));
			_r_obj_appendstringbuilder (&sb, SZ_CRLF);
		}

		if (ptr_app->is_silent)
		{
			_r_obj_appendstringbuilder (&sb, SZ_TAB);
			_r_obj_appendstringbuilder (&sb, _r_locale_getstring (IDS_HIGHLIGHT_SILENT));
			_r_obj_appendstringbuilder (&sb, SZ_CRLF);
		}

		if (ptr_app->is_undeletable)
		{
			_r_obj_appendstringbuilder (&sb, SZ_TAB);
			_r_obj_appendstringbuilder (&sb, _r_locale_getstring (IDS_DISABLEREMOVAL));
			_r_obj_appendstringbuilder (&sb, SZ_CRLF);
		}

		if (!_app_isappexists (ptr_app))
		{
			_r_obj_appendstringbuilder (&sb, SZ_TAB);
			_r_obj_appendstringbuilder (&sb, _r_locale_getstring (IDS_HIGHLIGHT_INVALID));
			_r_obj_appendstringbuilder (&sb, SZ_CRLF);
		}

		string = _r_obj_finalstringbuilder (&sb);

		if (!_r_obj_isstringempty2 (string))
		{
			_r_obj_insertstringbuilder (&sb, 0, L":\r\n");
			_r_obj_insertstringbuilder (&sb, 0, _r_locale_getstring (IDS_NOTES));

			_r_obj_appendstringbuilder2 (buffer, &string->sr);
		}

		_r_obj_deletestringbuilder (&sb);

		_r_obj_dereference (ptr_app);
	}

	// show additional log information
	if (ptr_log)
	{
		_r_obj_appendstringbuilder (buffer, _r_locale_getstring (IDS_TITLE_ADVANCED));
		_r_obj_appendstringbuilder (buffer, L":\r\n");

		_r_obj_appendstringbuilder (buffer, SZ_TAB);

		_r_obj_appendstringbuilder (buffer, _r_locale_getstring (IDS_FILTER));
		_r_obj_appendstringbuilder (buffer, L": ");

		if (ptr_log->filter_name)
		{
			_r_obj_appendstringbuilder2 (buffer, &ptr_log->filter_name->sr);
		}
		else
		{
			_r_obj_appendstringbuilder (buffer, SZ_EMPTY);
		}

		_r_obj_appendstringbuilder (buffer, SZ_CRLF SZ_TAB);

		_r_obj_appendstringbuilder (buffer, _r_locale_getstring (IDS_LAYER));
		_r_obj_appendstringbuilder (buffer, L": ");

		if (ptr_log->layer_name)
		{
			_r_obj_appendstringbuilder2 (buffer, &ptr_log->layer_name->sr);
		}
		else
		{
			_r_obj_appendstringbuilder (buffer, SZ_EMPTY);
		}

		_r_obj_appendstringbuilder (buffer, SZ_CRLF);
	}
}

_Ret_maybenull_
PR_STRING _app_gettooltipbylparam (
	_In_ HWND hwnd,
	_In_ INT listview_id,
	_In_ ULONG_PTR lparam
)
{
	R_STRINGBUILDER sb;
	PR_STRING string1;
	PR_STRING string2;
	PITEM_RULE ptr_rule;
	PITEM_NETWORK ptr_network;
	PITEM_LOG ptr_log;

	UNREFERENCED_PARAMETER (hwnd);

	_r_obj_initializestringbuilder (&sb, 512);

	switch (listview_id)
	{
		case IDC_APPS_PROFILE:
		case IDC_APPS_SERVICE:
		case IDC_APPS_UWP:
		case IDC_RULE_APPS_ID:
		{
			_app_getapptooltipstring (&sb, lparam, NULL, NULL);
			break;
		}

		case IDC_RULES_BLOCKLIST:
		case IDC_RULES_SYSTEM:
		case IDC_RULES_CUSTOM:
		case IDC_APP_RULES_ID:
		{
			ptr_rule = _app_getrulebyid (lparam);

			if (!ptr_rule)
				break;

			// rule address
			string1 = _app_rulesexpandrules (ptr_rule->rule_remote, SZ_CRLF SZ_TAB);
			string2 = _app_rulesexpandrules (ptr_rule->rule_local, SZ_CRLF SZ_TAB);

			_r_obj_appendstringbuilderformat (
				&sb,
				L"%s (#%" TEXT (PR_ULONG_PTR) L")\r\n%s (" SZ_DIRECTION_REMOTE L"):\r\n%s%s\r\n%s (" SZ_DIRECTION_LOCAL L"):\r\n%s%s",
				_r_obj_getstringordefault (ptr_rule->name, SZ_EMPTY),
				lparam,
				_r_locale_getstring (IDS_RULE),
				SZ_TAB,
				_r_obj_getstringordefault (string1, SZ_EMPTY),
				_r_locale_getstring (IDS_RULE),
				SZ_TAB,
				_r_obj_getstringordefault (string2, SZ_EMPTY)
			);

			if (string1)
				_r_obj_dereference (string1);

			if (string2)
				_r_obj_dereference (string2);

			// rule apps
			if (ptr_rule->is_forservices || !_r_obj_isempty (ptr_rule->apps))
			{
				string1 = _app_rulesexpandapps (ptr_rule, TRUE, SZ_TAB_CRLF);

				if (string1)
				{
					string2 = _r_obj_concatstrings (
						4,
						SZ_CRLF,
						_r_locale_getstring (IDS_TAB_APPS),
						L":\r\n" SZ_TAB,
						string1->buffer
					);

					_r_obj_appendstringbuilder2 (&sb, &string2->sr);
					_r_obj_appendstringbuilder (&sb, SZ_CRLF);

					_r_obj_dereference (string1);
					_r_obj_dereference (string2);
				}
			}

			// comment
			if (!_r_obj_isstringempty (ptr_rule->comment))
			{
				string1 = _r_obj_concatstrings (
					3,
					SZ_CRLF,
					_r_locale_getstring (IDS_COMMENT),
					L":\r\n" SZ_TAB
				);

				_r_obj_appendstringbuilder2 (&sb, &string1->sr);

				_r_obj_appendstringbuilder2 (&sb, &ptr_rule->comment->sr);

				_r_obj_dereference (string1);
			}

			// rule notes
			if (ptr_rule->is_readonly && ptr_rule->type == DATA_RULE_USER)
			{
				string2 = _r_obj_concatstrings (
					4,
					SZ_CRLF,
					_r_locale_getstring (IDS_NOTES),
					L":\r\n" SZ_TAB,
					SZ_RULE_INTERNAL_TITLE
				);

				_r_obj_appendstringbuilder2 (&sb, &string2->sr);

				_r_obj_dereference (string2);
			}

			_r_obj_dereference (ptr_rule);

			break;
		}

		case IDC_NETWORK:
		{
			ptr_network = _app_network_getitem (lparam);

			if (!ptr_network)
				break;

			_app_getapptooltipstring (&sb, ptr_network->app_hash, ptr_network, NULL);

			_r_obj_dereference (ptr_network);

			break;
		}

		case IDC_LOG:
		{
			ptr_log = _app_getlogitem (lparam);

			if (!ptr_log)
				break;

			_app_getapptooltipstring (&sb, ptr_log->app_hash, NULL, ptr_log);

			_r_obj_dereference (ptr_log);

			break;
		}
	}

	string1 = _r_obj_finalstringbuilder (&sb);

	if (!_r_obj_isstringempty2 (string1))
		return string1;

	_r_obj_deletestringbuilder (&sb);

	return NULL;
}

VOID _app_settab_id (
	_In_ HWND hwnd,
	_In_ INT page_id
)
{
	HWND hctrl;
	INT item_count;
	INT listview_id;

	if (!page_id)
		return;

	hctrl = GetDlgItem (hwnd, page_id);

	if (!hctrl || (_app_listview_getcurrent (hwnd) == page_id && _r_wnd_isvisible (hctrl, FALSE)))
		return;

	item_count = _r_tab_getitemcount (hwnd, IDC_TAB);

	if (!item_count)
		return;

	for (INT i = 0; i < item_count; i++)
	{
		listview_id = _app_listview_getbytab (hwnd, i);

		if (listview_id == page_id)
		{
			_r_tab_selectitem (hwnd, IDC_TAB, i);

			return;
		}
	}

	if (page_id != IDC_APPS_PROFILE)
		_app_settab_id (hwnd, IDC_APPS_PROFILE);
}

LPWSTR _app_getstateaction (
	_In_ ENUM_INSTALL_TYPE install_type
)
{
	UINT locale_id;

	switch (install_type)
	{
		case INSTALL_ENABLED:
		case INSTALL_ENABLED_TEMPORARY:
		{
			locale_id = IDS_TRAY_STOP;
			break;
		}

		case INSTALL_DISABLED:
		{
			locale_id = IDS_TRAY_START;
			break;
		}

		default:
		{
			return NULL;
		}
	}

	return _r_locale_getstring (locale_id);
}

HBITMAP _app_getstatebitmap (
	_In_ ENUM_INSTALL_TYPE install_type
)
{
	switch (install_type)
	{
		case INSTALL_ENABLED:
		case INSTALL_ENABLED_TEMPORARY:
		{
			return config.hbmp_disable;
		}

		case INSTALL_DISABLED:
		{
			return config.hbmp_enable;
		}
	}

	return NULL;
}

INT _app_getstateicon (
	_In_ ENUM_INSTALL_TYPE install_type
)
{
	switch (install_type)
	{
		case INSTALL_ENABLED:
		case INSTALL_ENABLED_TEMPORARY:
		{
			return IDI_ACTIVE;
		}
	}

	return IDI_INACTIVE;
}

LPCWSTR _app_getstatelocale (
	_In_ ENUM_INSTALL_TYPE install_type
)
{
	UINT locale_id;

	switch (install_type)
	{
		case INSTALL_ENABLED:
		{
			locale_id = IDS_STATUS_FILTERS_ACTIVE;
			break;
		}

		case INSTALL_ENABLED_TEMPORARY:
		{
			locale_id = IDS_STATUS_FILTERS_ACTIVE_TEMP;
			break;
		}

		case INSTALL_DISABLED:
		{
			locale_id = IDS_STATUS_FILTERS_INACTIVE;
			break;
		}

		default:
		{
			return NULL;
		}
	}

	return _r_locale_getstring (locale_id);
}

BOOLEAN _app_initinterfacestate (
	_In_ HWND hwnd,
	_In_ BOOLEAN is_forced
)
{
	if (is_forced || _r_toolbar_isenabled (config.hrebar, IDC_TOOLBAR, IDM_TRAY_START))
	{
		_r_toolbar_enablebutton (config.hrebar, IDC_TOOLBAR, IDM_TRAY_START, FALSE);
		_r_toolbar_enablebutton (config.hrebar, IDC_TOOLBAR, IDM_REFRESH, FALSE);

		_r_status_settextformat (hwnd, IDC_STATUSBAR, 0, L"%s...", _r_locale_getstring (IDS_STATUS_FILTERS_PROCESSING));

		return TRUE;
	}

	return FALSE;
}

VOID _app_restoreinterfacestate (
	_In_ HWND hwnd,
	_In_ BOOLEAN is_enabled
)
{
	ENUM_INSTALL_TYPE install_type;

	if (!is_enabled)
		return;

	install_type = _wfp_getinstalltype ();

	_r_toolbar_enablebutton (config.hrebar, IDC_TOOLBAR, IDM_TRAY_START, TRUE);
	_r_toolbar_enablebutton (config.hrebar, IDC_TOOLBAR, IDM_REFRESH, TRUE);

	_r_status_settext (hwnd, IDC_STATUSBAR, 0, _app_getstatelocale (install_type));
}

VOID _app_setinterfacestate (
	_In_ HWND hwnd,
	_In_ LONG dpi_value
)
{
	ENUM_INSTALL_TYPE install_type;
	HICON hico_sm;
	HICON hico_big;
	LONG icon_small;
	LONG icon_large;
	INT icon_id;
	BOOLEAN is_filtersinstalled;

	install_type = _wfp_getinstalltype ();
	is_filtersinstalled = (install_type != INSTALL_DISABLED);

	icon_small = _r_dc_getsystemmetrics (SM_CXSMICON, dpi_value);
	icon_large = _r_dc_getsystemmetrics (SM_CXICON, dpi_value);

	icon_id = _app_getstateicon (install_type);

	hico_sm = _r_sys_loadsharedicon (_r_sys_getimagebase (), MAKEINTRESOURCEW (icon_id), icon_small);
	hico_big = _r_sys_loadsharedicon (_r_sys_getimagebase (), MAKEINTRESOURCEW (icon_id), icon_large);

	_r_wnd_seticon (hwnd, hico_sm, hico_big);

	//_r_wnd_sendmessage (hwnd, IDC_STATUSBAR, SB_SETICON, 0, (LPARAM)hico_sm);

	if (!_wfp_isfiltersapplying ())
		_r_status_settext (hwnd, IDC_STATUSBAR, 0, _app_getstatelocale (install_type));

	_r_toolbar_setbutton (
		config.hrebar,
		IDC_TOOLBAR,
		IDM_TRAY_START,
		_app_getstateaction (install_type),
		BTNS_BUTTON | BTNS_AUTOSIZE | BTNS_SHOWTEXT,
		0,
		is_filtersinstalled ? 1 : 0
	);

	_app_settrayicon (hwnd, install_type);
}

VOID _app_settrayicon (
	_In_ HWND hwnd,
	_In_ ENUM_INSTALL_TYPE install_type
)
{
	HICON current_handle;
	HICON new_handle;
	LONG dpi_value;
	LONG icon_size;
	INT icon_id;
	HRESULT status;

	dpi_value = _r_dc_gettaskbardpi ();
	icon_id = _app_getstateicon (install_type);
	icon_size = _r_dc_getsystemmetrics (SM_CXSMICON, dpi_value);

	current_handle = _InterlockedCompareExchangePointer (&config.htray_icon, NULL, config.htray_icon);

	if (current_handle)
		DestroyIcon (current_handle);

	status = _r_sys_loadicon (_r_sys_getimagebase (), MAKEINTRESOURCEW (icon_id), icon_size, &new_handle);

	if (SUCCEEDED (status))
	{
		_r_tray_setinfo (hwnd, &GUID_TrayIcon, new_handle, _r_app_getname ());

		current_handle = _InterlockedCompareExchangePointer (&config.htray_icon, new_handle, NULL);

		if (current_handle)
			DestroyIcon (current_handle);
	}
}

VOID _app_imagelist_init (
	_In_opt_ HWND hwnd,
	_In_ LONG dpi_value
)
{
	static UINT toolbar_ids[] = {
		IDP_SHIELD_ENABLE,
		IDP_SHIELD_DISABLE,
		IDP_REFRESH,
		IDP_SETTINGS,
		IDP_NOTIFICATIONS,
		IDP_LOG,
		IDP_LOGOPEN,
		IDP_LOGCLEAR,
		IDP_ADD,
		IDP_DONATE,
		IDP_LOGUI,
	};

	static UINT rules_ids[] = {
		IDP_ALLOW,
		IDP_BLOCK
	};

	HBITMAP hbitmap;
	LONG icon_size_toolbar;
	LONG icon_small;
	LONG icon_large;

	SAFE_DELETE_OBJECT (config.hbmp_enable);
	SAFE_DELETE_OBJECT (config.hbmp_disable);
	SAFE_DELETE_OBJECT (config.hbmp_allow);
	SAFE_DELETE_OBJECT (config.hbmp_block);

	icon_small = _r_dc_getsystemmetrics (SM_CXSMICON, dpi_value);
	icon_large = _r_dc_getsystemmetrics (SM_CXICON, dpi_value);

	icon_size_toolbar = _r_calc_clamp (_r_dc_getdpi (_r_config_getlong (L"ToolbarSize", PR_SIZE_ITEMHEIGHT), dpi_value), icon_small, icon_large);

	_r_res_loadimage (_r_sys_getimagebase (), L"PNG", MAKEINTRESOURCEW (IDP_SHIELD_ENABLE), &GUID_ContainerFormatPng, icon_small, icon_small, &config.hbmp_enable);
	_r_res_loadimage (_r_sys_getimagebase (), L"PNG", MAKEINTRESOURCEW (IDP_SHIELD_DISABLE), &GUID_ContainerFormatPng, icon_small, icon_small, &config.hbmp_disable);

	_r_res_loadimage (_r_sys_getimagebase (), L"PNG", MAKEINTRESOURCEW (IDP_ALLOW), &GUID_ContainerFormatPng, icon_small, icon_small, &config.hbmp_allow);
	_r_res_loadimage (_r_sys_getimagebase (), L"PNG", MAKEINTRESOURCEW (IDP_BLOCK), &GUID_ContainerFormatPng, icon_small, icon_small, &config.hbmp_block);

	// toolbar imagelist
	if (config.himg_toolbar)
	{
		_r_imagelist_setsize (config.himg_toolbar, icon_size_toolbar);
	}
	else
	{
		_r_imagelist_create (icon_size_toolbar, icon_size_toolbar, ILC_COLOR32 | ILC_HIGHQUALITYSCALE, RTL_NUMBER_OF (toolbar_ids), RTL_NUMBER_OF (toolbar_ids), &config.himg_toolbar);
	}

	if (config.himg_toolbar)
	{
		for (ULONG_PTR i = 0; i < RTL_NUMBER_OF (toolbar_ids); i++)
		{
			_r_res_loadimage (_r_sys_getimagebase (), L"PNG", MAKEINTRESOURCEW (toolbar_ids[i]), &GUID_ContainerFormatPng, icon_size_toolbar, icon_size_toolbar, &hbitmap);

			if (hbitmap)
				_r_imagelist_add (config.himg_toolbar, hbitmap, NULL, NULL);
		}
	}

	if (config.htoolbar)
		_r_toolbar_setimagelist (config.htoolbar, 0, config.himg_toolbar);

	// rules imagelist (small)
	if (config.himg_rules_small)
	{
		_r_imagelist_setsize (config.himg_rules_small, icon_small);
	}
	else
	{
		_r_imagelist_create (icon_small, icon_small, ILC_COLOR32 | ILC_HIGHQUALITYSCALE, RTL_NUMBER_OF (rules_ids), RTL_NUMBER_OF (rules_ids), &config.himg_rules_small);
	}

	if (config.himg_rules_small)
	{
		for (ULONG_PTR i = 0; i < RTL_NUMBER_OF (rules_ids); i++)
		{
			_r_res_loadimage (_r_sys_getimagebase (), L"PNG", MAKEINTRESOURCEW (rules_ids[i]), &GUID_ContainerFormatPng, icon_small, icon_small, &hbitmap);

			if (hbitmap)
				_r_imagelist_add (config.himg_rules_small, hbitmap, NULL, NULL);
		}
	}

	// rules imagelist (large)
	if (config.himg_rules_large)
	{
		_r_imagelist_setsize (config.himg_rules_large, icon_large);
	}
	else
	{
		_r_imagelist_create (icon_large, icon_large, ILC_COLOR32 | ILC_HIGHQUALITYSCALE, RTL_NUMBER_OF (rules_ids), RTL_NUMBER_OF (rules_ids), &config.himg_rules_large);
	}

	if (config.himg_rules_large)
	{
		for (ULONG_PTR i = 0; i < RTL_NUMBER_OF (rules_ids); i++)
		{
			_r_res_loadimage (_r_sys_getimagebase (), L"PNG", MAKEINTRESOURCEW (rules_ids[i]), &GUID_ContainerFormatPng, icon_large, icon_large, &hbitmap);

			if (hbitmap)
				_r_imagelist_add (config.himg_rules_large, hbitmap, NULL, NULL);
		}
	}
}

HFONT _app_createfont (
	_Inout_ PLOGFONT logfont,
	_In_ LONG size,
	_In_ BOOLEAN is_underline,
	_In_ LONG dpi_value
)
{
	HFONT hfont;

	if (size)
		logfont->lfHeight = _r_dc_fontsizetoheight (size, dpi_value);

	logfont->lfUnderline = is_underline;
	logfont->lfCharSet = DEFAULT_CHARSET;
	logfont->lfQuality = DEFAULT_QUALITY;

	hfont = CreateFontIndirectW (logfont);

	return hfont;
}

VOID _app_windowloadfont (
	_In_ LONG dpi_value
)
{
	NONCLIENTMETRICS ncm = {0};

	ncm.cbSize = sizeof (ncm);

	if (!_r_dc_getsystemparametersinfo (SPI_GETNONCLIENTMETRICS, ncm.cbSize, &ncm, dpi_value))
		return;

	SAFE_DELETE_OBJECT (config.wnd_font);

	config.wnd_font = _app_createfont (&ncm.lfMessageFont, 0, FALSE, 0);

	_app_toolbar_setfont ();
}

VOID _app_toolbar_init (
	_In_ HWND hwnd,
	_In_ LONG dpi_value
)
{
	REBARINFO ri = {0};
	ULONG button_size;
	LONG rebar_height;

	config.hrebar = GetDlgItem (hwnd, IDC_REBAR);

	_app_windowloadfont (dpi_value);

	ri.cbSize = sizeof (REBARINFO);

	_r_wnd_sendmessage (config.hrebar, 0, RB_SETBARINFO, 0, (LPARAM)&ri);

	config.htoolbar = CreateWindowExW (
		0,
		TOOLBARCLASSNAMEW,
		NULL,
		WS_CHILD | WS_VISIBLE | CCS_NOPARENTALIGN | CCS_NODIVIDER | TBSTYLE_FLAT | TBSTYLE_LIST | TBSTYLE_TRANSPARENT | TBSTYLE_TOOLTIPS | TBSTYLE_AUTOSIZE,
		CW_USEDEFAULT,
		CW_USEDEFAULT,
		CW_USEDEFAULT,
		CW_USEDEFAULT,
		config.hrebar,
		(HMENU)IDC_TOOLBAR,
		_r_sys_getimagebase (),
		NULL
	);

	if (config.htoolbar)
	{
		_r_toolbar_setstyle (config.hrebar, IDC_TOOLBAR, TBSTYLE_EX_DOUBLEBUFFER | TBSTYLE_EX_MIXEDBUTTONS | TBSTYLE_EX_HIDECLIPPEDBUTTONS);

		_r_ctrl_setfont (config.htoolbar, 0, config.wnd_font); // fix font
		_r_toolbar_setimagelist (config.htoolbar, 0, config.himg_toolbar);

		_r_toolbar_addbutton (config.hrebar, IDC_TOOLBAR, IDM_TRAY_START, 0, BTNS_BUTTON | BTNS_AUTOSIZE, TBSTATE_ENABLED, I_IMAGENONE);

		_r_toolbar_addseparator (config.hrebar, IDC_TOOLBAR);

		_r_toolbar_addbutton (config.hrebar, IDC_TOOLBAR, IDM_OPENRULESEDITOR, 0, BTNS_BUTTON | BTNS_AUTOSIZE, TBSTATE_ENABLED, 8);

		_r_toolbar_addseparator (config.hrebar, IDC_TOOLBAR);

		_r_toolbar_addbutton (config.hrebar, IDC_TOOLBAR, IDM_TRAY_ENABLENOTIFICATIONS_CHK, 0, BTNS_BUTTON | BTNS_AUTOSIZE, TBSTATE_ENABLED, 4);
		_r_toolbar_addbutton (config.hrebar, IDC_TOOLBAR, IDM_TRAY_ENABLELOG_CHK, 0, BTNS_BUTTON | BTNS_AUTOSIZE, TBSTATE_ENABLED, 5);
		_r_toolbar_addbutton (config.hrebar, IDC_TOOLBAR, IDM_TRAY_ENABLEUILOG_CHK, 0, BTNS_BUTTON | BTNS_AUTOSIZE, TBSTATE_ENABLED, 10);

		_r_toolbar_addseparator (config.hrebar, IDC_TOOLBAR);

		_r_toolbar_addbutton (config.hrebar, IDC_TOOLBAR, IDM_REFRESH, 0, BTNS_BUTTON | BTNS_AUTOSIZE, TBSTATE_ENABLED, 2);
		_r_toolbar_addbutton (config.hrebar, IDC_TOOLBAR, IDM_SETTINGS, 0, BTNS_BUTTON | BTNS_AUTOSIZE, TBSTATE_ENABLED, 3);

		_r_toolbar_addseparator (config.hrebar, IDC_TOOLBAR);

		_r_toolbar_addbutton (config.hrebar, IDC_TOOLBAR, IDM_TRAY_LOGSHOW, 0, BTNS_BUTTON | BTNS_AUTOSIZE, TBSTATE_ENABLED, 6);
		_r_toolbar_addbutton (config.hrebar, IDC_TOOLBAR, IDM_TRAY_LOGCLEAR, 0, BTNS_BUTTON | BTNS_AUTOSIZE, TBSTATE_ENABLED, 7);

		_r_toolbar_addseparator (config.hrebar, IDC_TOOLBAR);

		_r_toolbar_addbutton (config.hrebar, IDC_TOOLBAR, IDM_DONATE, 0, BTNS_BUTTON | BTNS_AUTOSIZE, TBSTATE_ENABLED, 9);

		_r_toolbar_resize (config.hrebar, IDC_TOOLBAR);

		// insert toolbar
		button_size = _r_toolbar_getbuttonsize (config.hrebar, IDC_TOOLBAR);

		_r_rebar_insertband (hwnd, IDC_REBAR, REBAR_TOOLBAR_ID, config.htoolbar, RBBS_VARIABLEHEIGHT | RBBS_NOGRIPPER | RBBS_USECHEVRON, LOWORD (button_size), HIWORD (button_size));
	}

	// insert searchbar
	config.hsearchbar = CreateWindowExW (
		WS_EX_CLIENTEDGE,
		WC_EDITW,
		NULL,
		WS_CHILD | WS_CLIPSIBLINGS | WS_CLIPCHILDREN | ES_LEFT | ES_AUTOHSCROLL,
		CW_USEDEFAULT,
		CW_USEDEFAULT,
		CW_USEDEFAULT,
		CW_USEDEFAULT,
		config.hrebar,
		(HMENU)IDC_SEARCH,
		_r_sys_getimagebase (),
		NULL
	);

	if (!config.hsearchbar)
		return;

	_r_ctrl_setfont (config.hsearchbar, 0, config.wnd_font); // fix font

	_app_search_create (config.hsearchbar);

	rebar_height = _r_rebar_getheight (hwnd, IDC_REBAR);

	_app_search_setvisible (hwnd, config.hsearchbar, dpi_value);
}

VOID _app_toolbar_resize (
	_In_ HWND hwnd,
	_In_ LONG dpi_value
)
{
	REBARBANDINFOW rbi;
	SIZE ideal_size = {0};
	ULONG button_size;
	UINT rebar_count;

	_app_toolbar_setfont ();

	_r_wnd_sendmessage (config.htoolbar, 0, TB_AUTOSIZE, 0, 0);

	rebar_count = (UINT)_r_wnd_sendmessage (config.hrebar, 0, RB_GETBANDCOUNT, 0, 0);

	for (UINT i = 0; i < rebar_count; i++)
	{
		RtlZeroMemory (&rbi, sizeof (rbi));

		rbi.cbSize = sizeof (rbi);
		rbi.fMask = RBBIM_ID | RBBIM_CHILD | RBBIM_IDEALSIZE | RBBIM_CHILDSIZE;

		if (!_r_wnd_sendmessage (config.hrebar, 0, RB_GETBANDINFO, (WPARAM)i, (LPARAM)&rbi))
			continue;

		if (rbi.wID == REBAR_TOOLBAR_ID)
		{
			if (!_r_wnd_sendmessage (config.htoolbar, 0, TB_GETIDEALSIZE, FALSE, (LPARAM)&ideal_size))
				continue;

			button_size = _r_toolbar_getbuttonsize (config.hrebar, IDC_TOOLBAR);

			rbi.cxIdeal = (UINT)ideal_size.cx;
			rbi.cxMinChild = LOWORD (button_size);
			rbi.cyMinChild = HIWORD (button_size);
		}
		else if (rbi.wID == REBAR_SEARCH_ID)
		{
			if (_r_wnd_isvisible (rbi.hwndChild, FALSE))
			{
				rbi.cxIdeal = (UINT)_r_dc_getdpi (180, dpi_value);
			}
			else
			{
				rbi.cxIdeal = 0;
			}

			rbi.cxMinChild = rbi.cxIdeal;
			rbi.cyMinChild = 20;
		}
		else
		{
			continue;
		}

		_r_wnd_sendmessage (config.hrebar, 0, RB_SETBANDINFO, (WPARAM)i, (LPARAM)&rbi);
	}
}

VOID _app_toolbar_setfont ()
{
	if (config.htoolbar)
		_r_ctrl_setfont (config.htoolbar, 0, config.wnd_font); // fix font

	if (config.hsearchbar)
		_r_ctrl_setfont (config.hsearchbar, 0, config.wnd_font); // fix font
}

VOID _app_window_resize (
	_In_ HWND hwnd,
	_In_ LPCRECT rect,
	_In_ LONG dpi_value
)
{
	HDWP hdefer;
	LONG rebar_height;
	LONG statusbar_height;
	INT current_listview_id;
	INT listview_id;
	INT tab_count;

	_app_toolbar_resize (hwnd, dpi_value);

	_r_wnd_sendmessage (config.hrebar, 0, WM_SIZE, 0, 0);
	_r_wnd_sendmessage (hwnd, IDC_STATUSBAR, WM_SIZE, 0, 0);

	current_listview_id = _app_listview_getbytab (hwnd, -1);

	rebar_height = _r_rebar_getheight (hwnd, IDC_REBAR);
	statusbar_height = _r_status_getheight (hwnd, IDC_STATUSBAR);

	hdefer = BeginDeferWindowPos (2);

	if (hdefer)
	{
		hdefer = DeferWindowPos (
			hdefer,
			config.hrebar,
			NULL,
			0,
			0,
			rect->right,
			rebar_height,
			SWP_NOZORDER | SWP_NOACTIVATE | SWP_NOOWNERZORDER
		);

		hdefer = DeferWindowPos (
			hdefer,
			GetDlgItem (hwnd, IDC_TAB),
			NULL,
			0,
			rebar_height,
			rect->right,
			rect->bottom - rebar_height - statusbar_height,
			SWP_NOZORDER | SWP_NOACTIVATE | SWP_NOOWNERZORDER
		);

		EndDeferWindowPos (hdefer);
	}

	tab_count = _r_tab_getitemcount (hwnd, IDC_TAB);

	for (INT i = 0; i < tab_count; i++)
	{
		listview_id = _app_listview_getbytab (hwnd, i);

		if (listview_id)
		{
			_r_tab_adjustchild (hwnd, IDC_TAB, GetDlgItem (hwnd, listview_id));

			if (listview_id == current_listview_id)
				_app_listview_resize (hwnd, listview_id, FALSE);
		}
	}

	_app_refreshstatus (hwnd);
}

VOID _app_refreshstatus (
	_In_ HWND hwnd
)
{
	ITEM_STATUS status = {0};
	PR_STRING string[STATUSBAR_PARTS_COUNT] = {0};
	LONG parts[STATUSBAR_PARTS_COUNT] = {0};
	LONG size[STATUSBAR_PARTS_COUNT] = {0};
	LONG calculated_width = 0;
	LONG spacing;
	HWND hstatus;
	HDC hdc;
	LONG dpi_value;

	hstatus = GetDlgItem (hwnd, IDC_STATUSBAR);

	if (!hstatus)
		return;

	hdc = GetDC (hstatus);

	if (!hdc)
		return;

	_r_dc_fixfont (hdc, hwnd, IDC_STATUSBAR); // fix

	_app_getcount (&status);

	dpi_value = _r_dc_getwindowdpi (hwnd);

	spacing = _r_dc_getdpi (16, dpi_value);

	for (ULONG_PTR i = 0; i < RTL_NUMBER_OF (parts); i++)
	{
		switch (i)
		{
			case 1:
			{
				string[i] = _r_format_string (L"%s: %" TEXT (PR_ULONG_PTR), _r_locale_getstring (IDS_STATUS_UNUSED_APPS), status.apps_unused_count);
				break;
			}

			case 2:
			{
				string[i] = _r_format_string (L"%s: %" TEXT (PR_ULONG_PTR), _r_locale_getstring (IDS_STATUS_TIMER_APPS), status.apps_timer_count);
				break;
			}
		}

		if (i)
		{
			if (string[i])
			{
				size[i] = _r_dc_getfontwidth (hdc, &string[i]->sr, NULL) + spacing;

				calculated_width += size[i];
			}
		}
	}

	parts[0] = _r_ctrl_getwidth (hwnd, IDC_STATUSBAR) - calculated_width - _r_dc_getsystemmetrics (SM_CXVSCROLL, dpi_value) - (_r_dc_getsystemmetrics (SM_CXBORDER, dpi_value) * 4);

	parts[1] = parts[0] + size[1];
	parts[2] = parts[1] + size[2];

	_r_status_setparts (hwnd, IDC_STATUSBAR, parts, RTL_NUMBER_OF (parts));

	for (ULONG_PTR i = 1; i < STATUSBAR_PARTS_COUNT; i++)
	{
		if (string[i])
		{
			_r_status_settext (hwnd, IDC_STATUSBAR, (LONG)i, string[i]->buffer);

			_r_obj_dereference (string[i]);
		}
	}

	ReleaseDC (hstatus, hdc);
}

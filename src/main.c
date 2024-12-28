// simplewall
// Copyright (c) 2016-2024 Henry++

#include "global.h"

BOOLEAN _app_changefilters (
	_In_ HWND hwnd,
	_In_ BOOLEAN is_install,
	_In_ BOOLEAN is_forced
)
{
	PITEM_CONTEXT context;
	INT listview_id;

	listview_id = _app_listview_getcurrent (hwnd);

	_app_listview_sort (hwnd, listview_id, INT_ERROR, FALSE);

	if (is_forced || _wfp_isfiltersinstalled ())
	{
		_app_initinterfacestate (hwnd, TRUE);

		context = _r_freelist_allocateitem (&context_free_list);

		context->hwnd = hwnd;
		context->is_install = is_install;

		_r_workqueue_queueitem (&wfp_queue, &_wfp_applythread, context);

		return TRUE;
	}

	_r_listview_redraw (hwnd, listview_id);

	_app_profile_save (hwnd);

	return FALSE;
}

BOOLEAN _app_installmessage (
	_In_opt_ HWND hwnd,
	_In_ BOOLEAN is_install
)
{
	TASKDIALOGCONFIG tdc = {0};
	WCHAR str_main[256];
	WCHAR radio_text_1[128];
	WCHAR radio_text_2[128];
	WCHAR str_flag[128];
	WCHAR str_button_text_1[64];
	WCHAR str_button_text_2[64];
	TASKDIALOG_BUTTON td_buttons[2] = {0};
	TASKDIALOG_BUTTON td_radios[2] = {0};
	INT command_id;
	INT radio_id;
	BOOL is_flagchecked;
	HRESULT status;

	tdc.cbSize = sizeof (tdc);
	tdc.dwFlags = TDF_ENABLE_HYPERLINKS | TDF_ALLOW_DIALOG_CANCELLATION | TDF_NO_SET_FOREGROUND;
	tdc.hwndParent = hwnd;
	tdc.pszWindowTitle = _r_app_getname ();
	tdc.pszMainIcon = is_install ? TD_INFORMATION_ICON : TD_WARNING_ICON;
	//tdc.dwCommonButtons = TDCBF_YES_BUTTON | TDCBF_NO_BUTTON;
	tdc.pfCallback = &_r_msg_callback;
	tdc.lpCallbackData = MAKELONG (0, TRUE); // on top

	if (_r_config_getboolean (L"WF_State", TRUE))
		tdc.dwFlags |= TDF_VERIFICATION_FLAG_CHECKED;

	tdc.pButtons = td_buttons;
	tdc.cButtons = RTL_NUMBER_OF (td_buttons);

	tdc.nDefaultButton = IDYES;

	_r_str_copy (
		str_button_text_1,
		RTL_NUMBER_OF (str_button_text_1),
		_app_getstateaction (is_install ? INSTALL_DISABLED : INSTALL_ENABLED)
	);

	_r_str_copy (str_button_text_2, RTL_NUMBER_OF (str_button_text_2), _r_locale_getstring (IDS_CLOSE));

	td_buttons[0].nButtonID = IDYES;
	td_buttons[0].pszButtonText = str_button_text_1;

	td_buttons[1].nButtonID = IDNO;
	td_buttons[1].pszButtonText = str_button_text_2;

	if (is_install)
	{
		_r_str_copy (str_main, RTL_NUMBER_OF (str_main), _r_locale_getstring (IDS_QUESTION_START));
		_r_str_copy (str_flag, RTL_NUMBER_OF (str_flag), _r_locale_getstring (IDS_DISABLEWINDOWSFIREWALL_CHK));

		tdc.pRadioButtons = td_radios;
		tdc.cRadioButtons = RTL_NUMBER_OF (td_radios);

		tdc.nDefaultRadioButton = IDYES;

		_r_str_copy (radio_text_1, RTL_NUMBER_OF (radio_text_1), _r_locale_getstring (IDS_INSTALL_PERMANENT));
		_r_str_copy (radio_text_2, RTL_NUMBER_OF (radio_text_2), _r_locale_getstring (IDS_INSTALL_TEMPORARY));

		td_radios[0].nButtonID = IDYES;
		td_radios[0].pszButtonText = radio_text_1;

		td_radios[1].nButtonID = IDNO;
		td_radios[1].pszButtonText = radio_text_2;
	}
	else
	{
		_r_str_copy (str_main, RTL_NUMBER_OF (str_main), _r_locale_getstring (IDS_QUESTION_STOP));
		_r_str_copy (str_flag, RTL_NUMBER_OF (str_flag), _r_locale_getstring (IDS_ENABLEWINDOWSFIREWALL_CHK));
	}

	tdc.pszMainInstruction = str_main;
	tdc.pszVerificationText = str_flag;

	status = _r_msg_taskdialog (&tdc, &command_id, &radio_id, &is_flagchecked);

	if (SUCCEEDED (status))
	{
		_r_config_setboolean (L"WF_State", is_flagchecked ? TRUE : FALSE);

		if (command_id == IDYES)
		{
			if (is_install)
			{
				config.is_filterstemporary = (radio_id == IDNO);

				if (is_flagchecked)
					_wfp_firewallenable (FALSE);
			}
			else
			{
				config.is_filterstemporary = FALSE;

				if (is_flagchecked)
					_wfp_firewallenable (TRUE);
			}

			return TRUE;
		}
	}

	return FALSE;
}

VOID _app_config_apply (
	_In_ HWND hwnd,
	_In_opt_ HWND hsettings,
	_In_ INT ctrl_id
)
{
	HMENU hmenu;
	BOOLEAN is_enabled;
	BOOLEAN new_val;

	switch (ctrl_id)
	{
		case IDC_LOADONSTARTUP_CHK:
		case IDM_LOADONSTARTUP_CHK:
		{
			new_val = !_r_autorun_isenabled ();
			break;
		}

		case IDC_STARTMINIMIZED_CHK:
		case IDM_STARTMINIMIZED_CHK:
		{
			new_val = !_r_config_getboolean (L"IsStartMinimized", FALSE);
			break;
		}

		case IDC_SKIPUACWARNING_CHK:
		case IDM_SKIPUACWARNING_CHK:
		{
			new_val = !_r_skipuac_isenabled ();
			break;
		}

		case IDC_CHECKUPDATES_CHK:
		case IDM_CHECKUPDATES_CHK:
		{
			new_val = !_r_update_isenabled (FALSE);
			break;
		}

		case IDC_RULE_BLOCKOUTBOUND:
		case IDM_RULE_BLOCKOUTBOUND:
		{
			new_val = !_r_config_getboolean (L"BlockOutboundConnections", TRUE);
			break;
		}

		case IDC_RULE_BLOCKINBOUND:
		case IDM_RULE_BLOCKINBOUND:
		{
			new_val = !_r_config_getboolean (L"BlockInboundConnections", TRUE);
			break;
		}

		case IDC_RULE_ALLOWLOOPBACK:
		case IDM_RULE_ALLOWLOOPBACK:
		{
			new_val = !_r_config_getboolean (L"AllowLoopbackConnections", TRUE);
			break;
		}

		case IDC_RULE_ALLOW6TO4:
		case IDM_RULE_ALLOW6TO4:
		{
			new_val = !_r_config_getboolean (L"AllowIPv6", TRUE);
			break;
		}

		case IDM_RULE_ALLOWWINDOWSUPDATE:
		{
			new_val = !_app_wufixenabled ();
			break;
		}

		case IDC_USESTEALTHMODE_CHK:
		{
			new_val = !_r_config_getboolean (L"UseStealthMode", TRUE);
			break;
		}

		case IDC_USECERTIFICATES_CHK:
		case IDM_USECERTIFICATES_CHK:
		{
			new_val = !_r_config_getboolean (L"IsCertificatesEnabled", TRUE);
			break;
		}

		case IDM_KEEPUNUSED_CHK:
		{
			new_val = !_r_config_getboolean (L"IsKeepUnusedApps", TRUE);
			break;
		}

		case IDC_USEHASHES_CHK:
		case IDM_USEHASHES_CHK:
		{
			new_val = !_r_config_getboolean (L"IsHashesEnabled", FALSE);
			break;
		}

		case IDC_INSTALLBOOTTIMEFILTERS_CHK:
		{
			new_val = !_r_config_getboolean (L"InstallBoottimeFilters", TRUE);
			break;
		}

		case IDC_USENETWORKRESOLUTION_CHK:
		case IDM_USENETWORKRESOLUTION_CHK:
		{
			new_val = !_r_config_getboolean (L"IsNetworkResolutionsEnabled", FALSE);
			break;
		}

		case IDM_USEAPPMONITOR_CHK:
		{
			new_val = !_r_config_getboolean (L"IsEnableAppMonitor", FALSE);
			break;
		}

		case IDM_USEDARKTHEME_CHK:
		{
			new_val = !_r_theme_isenabled ();
			break;
		}

		case IDM_PROFILETYPE_PLAIN:
		case IDM_PROFILETYPE_COMPRESSED:
		case IDM_PROFILETYPE_ENCRYPTED:
		{
			break;
		}

		default:
		{
			return;
		}
	}

	hmenu = GetMenu (hwnd);

	if (!hmenu)
		return;

	switch (ctrl_id)
	{
		case IDC_LOADONSTARTUP_CHK:
		case IDM_LOADONSTARTUP_CHK:
		{
			_r_autorun_enable (hsettings ? hsettings : hwnd, new_val);

			is_enabled = _r_autorun_isenabled ();

			_r_menu_checkitem (hmenu, IDM_LOADONSTARTUP_CHK, 0, MF_BYCOMMAND, is_enabled);

			if (hsettings)
				_r_ctrl_checkbutton (hsettings, IDC_LOADONSTARTUP_CHK, is_enabled);

			break;
		}

		case IDC_STARTMINIMIZED_CHK:
		case IDM_STARTMINIMIZED_CHK:
		{
			_r_config_setboolean (L"IsStartMinimized", new_val);

			_r_menu_checkitem (hmenu, IDM_STARTMINIMIZED_CHK, 0, MF_BYCOMMAND, new_val);

			break;
		}

		case IDC_SKIPUACWARNING_CHK:
		case IDM_SKIPUACWARNING_CHK:
		{
			_r_skipuac_enable (hsettings ? hsettings : hwnd, new_val);

			is_enabled = _r_skipuac_isenabled ();

			_r_menu_checkitem (hmenu, IDM_SKIPUACWARNING_CHK, 0, MF_BYCOMMAND, is_enabled);

			if (hsettings)
				_r_ctrl_checkbutton (hsettings, IDC_SKIPUACWARNING_CHK, is_enabled);

			break;
		}

		case IDC_CHECKUPDATES_CHK:
		case IDM_CHECKUPDATES_CHK:
		{
			_r_update_enable (new_val);

			_r_menu_checkitem (hmenu, IDM_CHECKUPDATES_CHK, 0, MF_BYCOMMAND, new_val);

			break;
		}

		case IDC_RULE_BLOCKOUTBOUND:
		case IDM_RULE_BLOCKOUTBOUND:
		{
			_r_config_setboolean (L"BlockOutboundConnections", new_val);

			_r_menu_checkitem (hmenu, IDM_RULE_BLOCKOUTBOUND, 0, MF_BYCOMMAND, new_val);

			break;
		}

		case IDC_RULE_BLOCKINBOUND:
		case IDM_RULE_BLOCKINBOUND:
		{
			_r_config_setboolean (L"BlockInboundConnections", new_val);

			_r_menu_checkitem (hmenu, IDM_RULE_BLOCKINBOUND, 0, MF_BYCOMMAND, new_val);

			break;
		}

		case IDC_RULE_ALLOWLOOPBACK:
		case IDM_RULE_ALLOWLOOPBACK:
		{
			_r_config_setboolean (L"AllowLoopbackConnections", new_val);

			_r_menu_checkitem (hmenu, IDM_RULE_ALLOWLOOPBACK, 0, MF_BYCOMMAND, new_val);

			break;
		}

		case IDC_RULE_ALLOW6TO4:
		case IDM_RULE_ALLOW6TO4:
		{
			_r_config_setboolean (L"AllowIPv6", new_val);

			_r_menu_checkitem (hmenu, IDM_RULE_ALLOW6TO4, 0, MF_BYCOMMAND, new_val);

			break;
		}

		case IDM_RULE_ALLOWWINDOWSUPDATE:
		{
			_r_config_setboolean (L"IsWUFixEnabled", new_val);

			_r_menu_checkitem (hmenu, IDM_RULE_ALLOWWINDOWSUPDATE, 0, MF_BYCOMMAND, new_val);

			break;
		}

		case IDM_PROFILETYPE_PLAIN:
		{
			_r_config_setlong (L"ProfileType", 0);

			_r_menu_checkitem (hmenu, IDM_PROFILETYPE_PLAIN, IDM_PROFILETYPE_ENCRYPTED, MF_BYCOMMAND, IDM_PROFILETYPE_PLAIN);

			_app_profile_save (hwnd);

			break;
		}

		case IDM_PROFILETYPE_COMPRESSED:
		{
			_r_config_setlong (L"ProfileType", 1);

			_r_menu_checkitem (hmenu, IDM_PROFILETYPE_PLAIN, IDM_PROFILETYPE_ENCRYPTED, MF_BYCOMMAND, IDM_PROFILETYPE_COMPRESSED);

			_app_profile_save (hwnd);

			break;
		}

		case IDM_PROFILETYPE_ENCRYPTED:
		{
			_r_config_setlong (L"ProfileType", 2);

			_r_menu_checkitem (hmenu, IDM_PROFILETYPE_PLAIN, IDM_PROFILETYPE_ENCRYPTED, MF_BYCOMMAND, IDM_PROFILETYPE_ENCRYPTED);

			_app_profile_save (hwnd);

			break;
		}

		case IDC_USESTEALTHMODE_CHK:
		{
			_r_config_setboolean (L"UseStealthMode", new_val);
			break;
		}

		case IDC_INSTALLBOOTTIMEFILTERS_CHK:
		{
			_r_config_setboolean (L"InstallBoottimeFilters", new_val);
			break;
		}

		case IDC_USECERTIFICATES_CHK:
		case IDM_USECERTIFICATES_CHK:
		{
			PITEM_APP ptr_app = NULL;
			ULONG_PTR enum_key = 0;
			INT listview_id;

			_r_config_setboolean (L"IsCertificatesEnabled", new_val);

			_r_menu_checkitem (hmenu, IDM_USECERTIFICATES_CHK, 0, MF_BYCOMMAND, new_val);

			if (new_val)
			{
				_r_queuedlock_acquireshared (&lock_apps);

				while (_r_obj_enumhashtablepointer (apps_table, &ptr_app, NULL, &enum_key))
				{
					if (!ptr_app->real_path)
						continue;

					listview_id = _app_listview_getbytype (ptr_app->type);

					_app_getfileinformation (ptr_app->real_path, ptr_app->app_hash, ptr_app->type, listview_id);
				}

				_r_queuedlock_releaseshared (&lock_apps);
			}

			break;
		}

		case IDM_KEEPUNUSED_CHK:
		{
			_r_config_setboolean (L"IsKeepUnusedApps", new_val);

			_r_menu_checkitem (hmenu, IDM_KEEPUNUSED_CHK, 0, MF_BYCOMMAND, new_val);

			break;
		}

		case IDC_USEHASHES_CHK:
		case IDM_USEHASHES_CHK:
		{
			PITEM_APP ptr_app = NULL;
			ULONG_PTR enum_key = 0;
			INT listview_id;

			_r_config_setboolean (L"IsHashesEnabled", new_val);

			_r_menu_checkitem (hmenu, IDM_USEHASHES_CHK, 0, MF_BYCOMMAND, new_val);

			_r_menu_enableitem (hmenu, IDM_USEAPPMONITOR_CHK, FALSE, new_val);

			if (new_val)
			{
				_r_queuedlock_acquireshared (&lock_apps);

				while (_r_obj_enumhashtablepointer (apps_table, &ptr_app, NULL, &enum_key))
				{
					if (!ptr_app->real_path)
						continue;

					listview_id = _app_listview_getbytype (ptr_app->type);

					_app_getfileinformation (ptr_app->real_path, ptr_app->app_hash, ptr_app->type, listview_id);
				}

				_r_queuedlock_releaseshared (&lock_apps);
			}

			break;
		}

		case IDC_USENETWORKRESOLUTION_CHK:
		case IDM_USENETWORKRESOLUTION_CHK:
		{
			_r_config_setboolean (L"IsNetworkResolutionsEnabled", new_val);

			_r_menu_checkitem (hmenu, IDM_USENETWORKRESOLUTION_CHK, 0, MF_BYCOMMAND, new_val);

			break;
		}

		case IDM_USEAPPMONITOR_CHK:
		{
			_r_config_setboolean (L"IsEnableAppMonitor", new_val);

			_r_menu_checkitem (hmenu, IDM_USEAPPMONITOR_CHK, 0, MF_BYCOMMAND, new_val);

			_app_fileloggingenable ();

			break;
		}

		case IDM_USEDARKTHEME_CHK:
		{
			_r_menu_checkitem (hmenu, IDM_USEDARKTHEME_CHK, 0, MF_BYCOMMAND, new_val);

			_r_theme_enable (hwnd, new_val);

			break;
		}
	}

	switch (ctrl_id)
	{
		case IDM_LOADONSTARTUP_CHK:
		case IDC_LOADONSTARTUP_CHK:
		case IDM_STARTMINIMIZED_CHK:
		case IDC_STARTMINIMIZED_CHK:
		case IDM_SKIPUACWARNING_CHK:
		case IDC_SKIPUACWARNING_CHK:
		case IDM_CHECKUPDATES_CHK:
		case IDC_CHECKUPDATES_CHK:
		case IDM_PROFILETYPE_PLAIN:
		case IDM_PROFILETYPE_COMPRESSED:
		case IDM_PROFILETYPE_ENCRYPTED:
		case IDC_USENETWORKRESOLUTION_CHK:
		case IDM_USENETWORKRESOLUTION_CHK:
		case IDC_USECERTIFICATES_CHK:
		case IDM_USECERTIFICATES_CHK:
		case IDM_KEEPUNUSED_CHK:
		case IDC_USEHASHES_CHK:
		case IDM_USEHASHES_CHK:
		case IDM_USEAPPMONITOR_CHK:
		case IDM_USEDARKTHEME_CHK:
		{
			return;
		}
	}

	if (_wfp_isfiltersinstalled ())
		_app_changefilters (hwnd, TRUE, TRUE);
}

INT_PTR CALLBACK SettingsProc (
	_In_ HWND hwnd,
	_In_ UINT msg,
	_In_ WPARAM wparam,
	_In_ LPARAM lparam
)
{
	switch (msg)
	{
		case RM_INITIALIZE:
		{
			INT dialog_id = (INT)wparam;

			switch (dialog_id)
			{
				case IDD_SETTINGS_GENERAL:
				{
					_r_ctrl_checkbutton (hwnd, IDC_ALWAYSONTOP_CHK, _r_config_getboolean (L"AlwaysOnTop", FALSE));

#if defined(APP_HAVE_AUTORUN)
					_r_ctrl_checkbutton (hwnd, IDC_LOADONSTARTUP_CHK, _r_autorun_isenabled ());
#endif // APP_HAVE_AUTORUN

					_r_ctrl_checkbutton (hwnd, IDC_STARTMINIMIZED_CHK, _r_config_getboolean (L"IsStartMinimized", FALSE));

#if defined(APP_HAVE_SKIPUAC)
					_r_ctrl_checkbutton (hwnd, IDC_SKIPUACWARNING_CHK, _r_skipuac_isenabled ());
#endif // APP_HAVE_SKIPUAC

					_r_ctrl_checkbutton (hwnd, IDC_CHECKUPDATES_CHK, _r_update_isenabled (FALSE));

					_r_locale_enum (hwnd, IDC_LANGUAGE, 0);

					break;
				}

				case IDD_SETTINGS_RULES:
				{
					HWND htip;

					_r_ctrl_checkbutton (hwnd, IDC_RULE_BLOCKOUTBOUND, _r_config_getboolean (L"BlockOutboundConnections", TRUE));
					_r_ctrl_checkbutton (hwnd, IDC_RULE_BLOCKINBOUND, _r_config_getboolean (L"BlockInboundConnections", TRUE));
					_r_ctrl_checkbutton (hwnd, IDC_RULE_ALLOWLOOPBACK, _r_config_getboolean (L"AllowLoopbackConnections", TRUE));
					_r_ctrl_checkbutton (hwnd, IDC_RULE_ALLOW6TO4, _r_config_getboolean (L"AllowIPv6", TRUE));
					_r_ctrl_checkbutton (hwnd, IDC_USESTEALTHMODE_CHK, _r_config_getboolean (L"UseStealthMode", TRUE));
					_r_ctrl_checkbutton (hwnd, IDC_INSTALLBOOTTIMEFILTERS_CHK, _r_config_getboolean (L"InstallBoottimeFilters", TRUE));
					_r_ctrl_checkbutton (hwnd, IDC_USECERTIFICATES_CHK, _r_config_getboolean (L"IsCertificatesEnabled", TRUE));
					_r_ctrl_checkbutton (hwnd, IDC_USEHASHES_CHK, _r_config_getboolean (L"IsHashesEnabled", FALSE));
					_r_ctrl_checkbutton (hwnd, IDC_USENETWORKRESOLUTION_CHK, _r_config_getboolean (L"IsNetworkResolutionsEnabled", FALSE));

					htip = _r_ctrl_createtip (hwnd);

					if (!htip)
						break;

					_r_ctrl_settiptext (htip, hwnd, IDC_RULE_BLOCKOUTBOUND, LPSTR_TEXTCALLBACK);
					_r_ctrl_settiptext (htip, hwnd, IDC_RULE_BLOCKINBOUND, LPSTR_TEXTCALLBACK);
					_r_ctrl_settiptext (htip, hwnd, IDC_RULE_ALLOWLOOPBACK, LPSTR_TEXTCALLBACK);
					_r_ctrl_settiptext (htip, hwnd, IDC_RULE_ALLOW6TO4, LPSTR_TEXTCALLBACK);

					_r_ctrl_settiptext (htip, hwnd, IDC_USESTEALTHMODE_CHK, LPSTR_TEXTCALLBACK);
					_r_ctrl_settiptext (htip, hwnd, IDC_INSTALLBOOTTIMEFILTERS_CHK, LPSTR_TEXTCALLBACK);

					break;
				}

				case IDD_SETTINGS_BLOCKLIST:
				{
					_r_ctrl_checkradio (
						hwnd,
						IDC_BLOCKLIST_SPY_DISABLE, IDC_BLOCKLIST_SPY_BLOCK, IDC_BLOCKLIST_SPY_DISABLE + _r_calc_clamp (_r_config_getlong (L"BlocklistSpyState", 2), 0, 2)
					);

					_r_ctrl_checkradio (
						hwnd,
						IDC_BLOCKLIST_UPDATE_DISABLE, IDC_BLOCKLIST_UPDATE_BLOCK, IDC_BLOCKLIST_UPDATE_DISABLE + _r_calc_clamp (_r_config_getlong (L"BlocklistUpdateState", 0), 0, 2)
					);

					_r_ctrl_checkradio (
						hwnd,
						IDC_BLOCKLIST_EXTRA_DISABLE, IDC_BLOCKLIST_EXTRA_BLOCK, IDC_BLOCKLIST_EXTRA_DISABLE + _r_calc_clamp (_r_config_getlong (L"BlocklistExtraState", 0), 0, 2)
					);

					break;
				}

				case IDD_SETTINGS_INTERFACE:
				{
					_r_ctrl_checkbutton (hwnd, IDC_CONFIRMEXIT_CHK, _r_config_getboolean (L"ConfirmExit2", TRUE));
					_r_ctrl_checkbutton (hwnd, IDC_CONFIRMEXITTIMER_CHK, _r_config_getboolean (L"ConfirmExitTimer", TRUE));
					_r_ctrl_checkbutton (hwnd, IDC_CONFIRMLOGCLEAR_CHK, _r_config_getboolean (L"ConfirmLogClear", TRUE));
					_r_ctrl_checkbutton (hwnd, IDC_CONFIRMALLOW_CHK, _r_config_getboolean (L"ConfirmAllow", TRUE));
					_r_ctrl_checkbutton (hwnd, IDC_TRAYICONSINGLECLICK_CHK, _r_config_getboolean (L"IsTrayIconSingleClick", TRUE));

					break;
				}

				case IDD_SETTINGS_HIGHLIGHTING:
				{
					PITEM_COLOR ptr_clr = NULL;
					ULONG_PTR enum_key = 0;
					BOOLEAN val;
					LONG icon_id;
					INT item_id = 0;

					// configure listview
					_r_listview_setstyle (hwnd, IDC_COLORS, LVS_EX_DOUBLEBUFFER | LVS_EX_FULLROWSELECT | LVS_EX_INFOTIP | LVS_EX_LABELTIP | LVS_EX_CHECKBOXES, FALSE);

					_app_listview_setview (hwnd, IDC_COLORS);

					_r_listview_deleteallitems (hwnd, IDC_COLORS);
					_r_listview_deleteallcolumns (hwnd, IDC_COLORS);

					_r_listview_addcolumn (hwnd, IDC_COLORS, 0, L"", 0, LVCFMT_LEFT);

					icon_id = _app_icons_getdefaultapp_id (DATA_APP_REGULAR);

					_app_listview_lock (hwnd, IDC_COLORS, TRUE);

					while (_r_obj_enumhashtable (colors_table, &ptr_clr, NULL, &enum_key))
					{
						ptr_clr->new_clr = _r_config_getulong_ex (ptr_clr->config_value->buffer, ptr_clr->default_clr, L"colors");

						_r_listview_additem (
							hwnd,
							IDC_COLORS,
							item_id,
							_r_locale_getstring (ptr_clr->locale_id),
							icon_id,
							I_GROUPIDNONE,
							(LPARAM)ptr_clr
						);

						val = _r_config_getboolean_ex (ptr_clr->config_name->buffer, ptr_clr->is_enabled, L"colors");

						_r_listview_setitemcheck (hwnd, IDC_COLORS, item_id, val);

						item_id += 1;
					}

					_app_listview_lock (hwnd, IDC_COLORS, FALSE);

					break;
				}

				case IDD_SETTINGS_NOTIFICATIONS:
				{
					_r_ctrl_checkbutton (hwnd, IDC_ENABLENOTIFICATIONS_CHK, _r_config_getboolean (L"IsNotificationsEnabled", TRUE));
					_r_ctrl_checkbutton (hwnd, IDC_NOTIFICATIONSOUND_CHK, _r_config_getboolean (L"IsNotificationsSound", TRUE));
					_r_ctrl_checkbutton (hwnd, IDC_NOTIFICATIONFULLSCREENSILENTMODE_CHK, _r_config_getboolean (L"IsNotificationsFullscreenSilentMode", TRUE));
					_r_ctrl_checkbutton (hwnd, IDC_NOTIFICATIONONTRAY_CHK, _r_config_getboolean (L"IsNotificationsOnTray", FALSE));

					_r_updown_setrange (hwnd, IDC_NOTIFICATIONTIMEOUT, 0, _r_calc_days2seconds (7));

					_r_updown_setvalue (hwnd, IDC_NOTIFICATIONTIMEOUT, _r_config_getulong (L"NotificationsTimeout", NOTIFY_TIMEOUT_DEFAULT));

					_r_ctrl_sendcommand (hwnd, IDC_ENABLENOTIFICATIONS_CHK, WM_APP);

					break;
				}

				case IDD_SETTINGS_LOGGING:
				{
					PR_STRING string;

					_r_ctrl_checkbutton (hwnd, IDC_ENABLELOG_CHK, _r_config_getboolean (L"IsLogEnabled", FALSE));

					string = _app_getlogpath ();

					if (string)
					{
						_r_ctrl_setstring (hwnd, IDC_LOGPATH, string->buffer);

						_r_obj_dereference (string);
					}

					string = _app_getlogviewer ();

					if (string)
					{
						_r_ctrl_setstring (hwnd, IDC_LOGVIEWER, string->buffer);

						_r_obj_dereference (string);
					}

					_r_ctrl_setacceleration (hwnd, IDC_LOGSIZELIMIT, 64); // set step to 64kb

					_r_updown_setrange (hwnd, IDC_LOGSIZELIMIT, 64, _r_calc_kilobytes2bytes (512));

					_r_updown_setvalue (hwnd, IDC_LOGSIZELIMIT, _r_config_getulong (L"LogSizeLimitKb", LOG_SIZE_LIMIT_DEFAULT));

					_r_ctrl_checkbutton (hwnd, IDC_ENABLEUILOG_CHK, _r_config_getboolean (L"IsLogUiEnabled", FALSE));

					_r_ctrl_sendcommand (hwnd, IDC_ENABLELOG_CHK, WM_APP);

					break;
				}

				case IDD_SETTINGS_EXCLUDE:
				{
					_r_ctrl_checkbutton (hwnd, IDC_EXCLUDEBLOCKLIST_CHK, _r_config_getboolean (L"IsExcludeBlocklist", TRUE));
					_r_ctrl_checkbutton (hwnd, IDC_EXCLUDECUSTOM_CHK, _r_config_getboolean (L"IsExcludeCustomRules", TRUE));
					_r_ctrl_checkbutton (hwnd, IDC_EXCLUDESTEALTH_CHK, _r_config_getboolean (L"IsExcludeStealth", TRUE));
					_r_ctrl_checkbutton (hwnd, IDC_EXCLUDECLASSIFYALLOW_CHK, _r_config_getboolean (L"IsExcludeClassifyAllow", TRUE));

					// win8+
					if (_r_sys_isosversionlower (WINDOWS_8))
						_r_ctrl_enable (hwnd, IDC_EXCLUDECLASSIFYALLOW_CHK, FALSE);

					break;
				}

				break;
			}

			break;
		}

		case RM_LOCALIZE:
		{
			INT dialog_id = (INT)wparam;

			// localize titles
			_r_ctrl_setstringformat (
				hwnd,
				IDC_TITLE_GENERAL,
				L"%s:",
				_r_locale_getstring (IDS_TITLE_GENERAL)
			);

			_r_ctrl_setstringformat (
				hwnd,
				IDC_TITLE_LANGUAGE,
				L"%s: (Language)",
				_r_locale_getstring (IDS_TITLE_LANGUAGE)
			);

			_r_ctrl_setstringformat (
				hwnd,
				IDC_TITLE_BLOCKLIST_SPY,
				L"%s:",
				_r_locale_getstring (IDS_BLOCKLIST_SPY)
			);

			_r_ctrl_setstringformat (
				hwnd,
				IDC_TITLE_BLOCKLIST_UPDATE,
				L"%s:",
				_r_locale_getstring (IDS_BLOCKLIST_UPDATE)
			);

			_r_ctrl_setstringformat (
				hwnd,
				IDC_TITLE_BLOCKLIST_EXTRA,
				L"%s: (Skype, Bing, Live, Outlook, etc.)",
				_r_locale_getstring (IDS_BLOCKLIST_EXTRA)
			);

			_r_ctrl_setstringformat (
				hwnd,
				IDC_TITLE_CONNECTIONS,
				L"%s:",
				_r_locale_getstring (IDS_TAB_NETWORK)
			);

			_r_ctrl_setstringformat (
				hwnd,
				IDC_TITLE_SECURITY,
				L"%s:",
				_r_locale_getstring (IDS_TITLE_SECURITY)
			);

			_r_ctrl_setstringformat (
				hwnd,
				IDC_TITLE_CONFIRMATIONS,
				L"%s:",
				_r_locale_getstring (IDS_TITLE_CONFIRMATIONS)
			);

			_r_ctrl_setstringformat (
				hwnd,
				IDC_TITLE_TRAY,
				L"%s:",
				_r_locale_getstring (IDS_TITLE_TRAY)
			);

			_r_ctrl_setstringformat (
				hwnd,
				IDC_TITLE_HIGHLIGHTING,
				L"%s:",
				_r_locale_getstring (IDS_TITLE_HIGHLIGHTING)
			);

			_r_ctrl_setstringformat (
				hwnd,
				IDC_TITLE_LOGVIEWER,
				L"%s:",
				_r_locale_getstring (IDS_LOGVIEWER_HINT)
			);

			_r_ctrl_setstringformat (
				hwnd,
				IDC_TITLE_INTERFACE,
				L"%s:",
				_r_locale_getstring (IDS_TITLE_INTERFACE)
			);

			_r_ctrl_setstringformat (
				hwnd,
				IDC_TITLE_NOTIFICATIONS,
				L"%s:",
				_r_locale_getstring (IDS_TITLE_NOTIFICATIONS)
			);

			_r_ctrl_setstringformat (
				hwnd, IDC_TITLE_LOGGING,
				L"%s:",
				_r_locale_getstring (IDS_TITLE_LOGGING)
			);

			_r_ctrl_setstringformat (
				hwnd,
				IDC_TITLE_ADVANCED,
				L"%s:",
				_r_locale_getstring (IDS_TITLE_ADVANCED)
			);

			switch (dialog_id)
			{
				case IDD_SETTINGS_GENERAL:
				{
					_r_ctrl_setstring (hwnd, IDC_ALWAYSONTOP_CHK, _r_locale_getstring (IDS_ALWAYSONTOP_CHK));
					_r_ctrl_setstring (hwnd, IDC_LOADONSTARTUP_CHK, _r_locale_getstring (IDS_LOADONSTARTUP_CHK));
					_r_ctrl_setstring (hwnd, IDC_STARTMINIMIZED_CHK, _r_locale_getstring (IDS_STARTMINIMIZED_CHK));
					_r_ctrl_setstring (hwnd, IDC_SKIPUACWARNING_CHK, _r_locale_getstring (IDS_SKIPUACWARNING_CHK));
					_r_ctrl_setstring (hwnd, IDC_CHECKUPDATES_CHK, _r_locale_getstring (IDS_CHECKUPDATES_CHK));

					_r_ctrl_setstring (hwnd, IDC_LANGUAGE_HINT, _r_locale_getstring (IDS_LANGUAGE_HINT));

					break;
				}

				case IDD_SETTINGS_RULES:
				{
					LPCWSTR recommended_string;

					recommended_string = _r_locale_getstring (IDS_RECOMMENDED);

					_r_ctrl_setstringformat (
						hwnd,
						IDC_RULE_BLOCKOUTBOUND,
						L"%s (%s)",
						_r_locale_getstring (IDS_RULE_BLOCKOUTBOUND),
						recommended_string
					);

					_r_ctrl_setstringformat (
						hwnd,
						IDC_RULE_BLOCKINBOUND,
						L"%s (%s)",
						_r_locale_getstring (IDS_RULE_BLOCKINBOUND),
						recommended_string
					);

					_r_ctrl_setstringformat (
						hwnd,
						IDC_RULE_ALLOWLOOPBACK,
						L"%s (%s)",
						_r_locale_getstring (IDS_RULE_ALLOWLOOPBACK),
						recommended_string
					);

					_r_ctrl_setstringformat (
						hwnd,
						IDC_RULE_ALLOW6TO4,
						L"%s (%s)",
						_r_locale_getstring (IDS_RULE_ALLOW6TO4),
						recommended_string
					);

					_r_ctrl_setstringformat (
						hwnd,
						IDC_USESTEALTHMODE_CHK,
						L"%s (%s)",
						_r_locale_getstring (IDS_USESTEALTHMODE_CHK),
						recommended_string
					);

					_r_ctrl_setstringformat (
						hwnd,
						IDC_INSTALLBOOTTIMEFILTERS_CHK,
						L"%s (%s)",
						_r_locale_getstring (IDS_INSTALLBOOTTIMEFILTERS_CHK),
						recommended_string
					);

					_r_ctrl_setstring (hwnd, IDC_USECERTIFICATES_CHK, _r_locale_getstring (IDS_USECERTIFICATES_CHK));
					_r_ctrl_setstring (hwnd, IDC_USEHASHES_CHK, _r_locale_getstring (IDS_USEHASHES_CHK));

					_r_ctrl_setstring (
						hwnd,
						IDC_USENETWORKRESOLUTION_CHK,
						_r_locale_getstring (IDS_USENETWORKRESOLUTION_CHK)
					);

					break;
				}

				case IDD_SETTINGS_BLOCKLIST:
				{
					LPCWSTR recommended_string;
					LPCWSTR disable_string;
					LPCWSTR allow_string;
					LPCWSTR block_string;

					recommended_string = _r_locale_getstring (IDS_RECOMMENDED);
					disable_string = _r_locale_getstring (IDS_DISABLE);
					allow_string = _r_locale_getstring (IDS_ACTION_ALLOW);
					block_string = _r_locale_getstring (IDS_ACTION_BLOCK);

					_r_ctrl_setstring (hwnd, IDC_BLOCKLIST_SPY_DISABLE, disable_string);
					_r_ctrl_setstring (hwnd, IDC_BLOCKLIST_SPY_ALLOW, allow_string);

					_r_ctrl_setstringformat (
						hwnd,
						IDC_BLOCKLIST_SPY_BLOCK,
						L"%s (%s)",
						block_string,
						recommended_string
					);

					_r_ctrl_setstringformat (
						hwnd,
						IDC_BLOCKLIST_UPDATE_DISABLE,
						L"%s (%s)",
						disable_string,
						recommended_string
					);

					_r_ctrl_setstring (hwnd, IDC_BLOCKLIST_UPDATE_ALLOW, allow_string);
					_r_ctrl_setstring (hwnd, IDC_BLOCKLIST_UPDATE_BLOCK, block_string);

					_r_ctrl_setstringformat (
						hwnd,
						IDC_BLOCKLIST_EXTRA_DISABLE,
						L"%s (%s)",
						disable_string,
						recommended_string
					);

					_r_ctrl_setstring (hwnd, IDC_BLOCKLIST_EXTRA_ALLOW, allow_string);
					_r_ctrl_setstring (hwnd, IDC_BLOCKLIST_EXTRA_BLOCK, block_string);

					_r_ctrl_setstringformat (
						hwnd,
						IDC_BLOCKLIST_INFO,
						L"Author: <a href=\"%s\">WindowsSpyBlocker</a> - block spying and tracking on Windows systems.",
						WINDOWSSPYBLOCKER_URL
					);

					break;
				}

				case IDD_SETTINGS_INTERFACE:
				{
					_r_ctrl_setstring (hwnd, IDC_CONFIRMEXIT_CHK, _r_locale_getstring (IDS_CONFIRMEXIT_CHK));
					_r_ctrl_setstring (hwnd, IDC_CONFIRMEXITTIMER_CHK, _r_locale_getstring (IDS_CONFIRMEXITTIMER_CHK));
					_r_ctrl_setstring (hwnd, IDC_CONFIRMLOGCLEAR_CHK, _r_locale_getstring (IDS_CONFIRMLOGCLEAR_CHK));
					_r_ctrl_setstring (hwnd, IDC_CONFIRMALLOW_CHK, _r_locale_getstring (IDS_CONFIRMALLOW_CHK));

					_r_ctrl_setstring (
						hwnd,
						IDC_TRAYICONSINGLECLICK_CHK,
						_r_locale_getstring (IDS_TRAYICONSINGLECLICK_CHK)
					);

					break;
				}

				case IDD_SETTINGS_HIGHLIGHTING:
				{
					PITEM_COLOR ptr_clr;

					_r_listview_setcolumn (hwnd, IDC_COLORS, 0, NULL, -100);

					for (INT i = 0; i < _r_listview_getitemcount (hwnd, IDC_COLORS); i++)
					{
						ptr_clr = (PITEM_COLOR)_r_listview_getitemlparam (hwnd, IDC_COLORS, i);

						if (ptr_clr)
							_r_listview_setitem (hwnd, IDC_COLORS, i, 0, _r_locale_getstring (ptr_clr->locale_id), I_DEFAULT, I_DEFAULT, I_DEFAULT);
					}

					_r_ctrl_setstring (hwnd, IDC_COLORS_HINT, _r_locale_getstring (IDS_COLORS_HINT));

					break;
				}

				case IDD_SETTINGS_NOTIFICATIONS:
				{
					_r_ctrl_setstring (
						hwnd,
						IDC_ENABLENOTIFICATIONS_CHK,
						_r_locale_getstring (IDS_ENABLENOTIFICATIONS_CHK)
					);

					_r_ctrl_setstring (
						hwnd,
						IDC_NOTIFICATIONSOUND_CHK,
						_r_locale_getstring (IDS_NOTIFICATIONSOUND_CHK)
					);

					_r_ctrl_setstring (
						hwnd,
						IDC_NOTIFICATIONFULLSCREENSILENTMODE_CHK,
						_r_locale_getstring (IDS_NOTIFICATIONFULLSCREENSILENTMODE_CHK)
					);

					_r_ctrl_setstring (
						hwnd,
						IDC_NOTIFICATIONONTRAY_CHK,
						_r_locale_getstring (IDS_NOTIFICATIONONTRAY_CHK)
					);

					_r_ctrl_setstring (
						hwnd,
						IDC_NOTIFICATIONTIMEOUT_HINT,
						_r_locale_getstring (IDS_NOTIFICATIONTIMEOUT_HINT)
					);

					break;
				}

				case IDD_SETTINGS_LOGGING:
				{
					_r_ctrl_setstring (hwnd, IDC_ENABLELOG_CHK, _r_locale_getstring (IDS_ENABLELOG_CHK));

					_r_ctrl_setstring (hwnd, IDC_LOGSIZELIMIT_HINT, _r_locale_getstring (IDS_LOGSIZELIMIT_HINT));

					_r_ctrl_setstringformat (
						hwnd,
						IDC_ENABLEUILOG_CHK,
						L"%s (session only)",
						_r_locale_getstring (IDS_ENABLEUILOG_CHK)
					);

					break;
				}

				case IDD_SETTINGS_EXCLUDE:
				{
					_r_ctrl_setstringformat (
						hwnd,
						IDC_EXCLUDEBLOCKLIST_CHK,
						L"%s %s",
						_r_locale_getstring (IDS_TITLE_EXCLUDE),
						_r_locale_getstring (IDS_EXCLUDEBLOCKLIST_CHK)
					);

					_r_ctrl_setstringformat (
						hwnd,
						IDC_EXCLUDECUSTOM_CHK,
						L"%s %s",
						_r_locale_getstring (IDS_TITLE_EXCLUDE),
						_r_locale_getstring (IDS_EXCLUDECUSTOM_CHK)
					);

					_r_ctrl_setstringformat (
						hwnd,
						IDC_EXCLUDESTEALTH_CHK,
						L"%s %s",
						_r_locale_getstring (IDS_TITLE_EXCLUDE),
						_r_locale_getstring (IDS_EXCLUDESTEALTH_CHK)
					);

					_r_ctrl_setstringformat (
						hwnd,
						IDC_EXCLUDECLASSIFYALLOW_CHK,
						L"%s %s [win8+]",
						_r_locale_getstring (IDS_TITLE_EXCLUDE),
						_r_locale_getstring (IDS_EXCLUDECLASSIFYALLOW_CHK)
					);

					break;
				}
			}

			break;
		}

		case WM_SIZE:
		{
			RECT rect;
			HWND hlistview;

			hlistview = GetDlgItem (hwnd, IDC_COLORS);

			if (!hlistview)
				break;

			if (GetClientRect (hlistview, &rect))
				_r_listview_setcolumn (hwnd, IDC_COLORS, 0, NULL, rect.right);

			break;
		}

		case WM_VSCROLL:
		case WM_HSCROLL:
		{
			LONG value;
			INT ctrl_id;

			ctrl_id = GetDlgCtrlID ((HWND)lparam);

			if (ctrl_id == IDC_LOGSIZELIMIT)
			{
				value = _r_updown_getvalue (hwnd, ctrl_id);

				_r_config_setlong (L"LogSizeLimitKb", value);
			}
			else if (ctrl_id == IDC_NOTIFICATIONTIMEOUT)
			{
				value = _r_updown_getvalue (hwnd, ctrl_id);

				_r_config_setlong (L"NotificationsTimeout", value);
			}

			break;
		}

		case WM_NOTIFY:
		{
			LPNMHDR nmlp;

			nmlp = (LPNMHDR)lparam;

			switch (nmlp->code)
			{
				case TTN_GETDISPINFO:
				{
					LPNMTTDISPINFOW lpnmdi = (LPNMTTDISPINFOW)lparam;
					WCHAR buffer[1024] = {0};
					INT locale_id;
					INT ctrl_id;

					if ((lpnmdi->uFlags & TTF_IDISHWND) != TTF_IDISHWND)
						break;

					ctrl_id = GetDlgCtrlID ((HWND)(lpnmdi->hdr.idFrom));

					if (ctrl_id == IDC_RULE_BLOCKOUTBOUND)
					{
						locale_id = IDS_RULE_BLOCKOUTBOUND;
					}
					else if (ctrl_id == IDC_RULE_BLOCKINBOUND)
					{
						locale_id = IDS_RULE_BLOCKINBOUND;
					}
					else if (ctrl_id == IDC_RULE_ALLOWLOOPBACK)
					{
						locale_id = IDS_RULE_ALLOWLOOPBACK_HINT;
					}
					else if (ctrl_id == IDC_USESTEALTHMODE_CHK)
					{
						locale_id = IDS_USESTEALTHMODE_HINT;
					}
					else if (ctrl_id == IDC_INSTALLBOOTTIMEFILTERS_CHK)
					{
						locale_id = IDS_INSTALLBOOTTIMEFILTERS_HINT;
					}
					else
					{
						break;
					}

					_r_str_copy (buffer, RTL_NUMBER_OF (buffer), _r_locale_getstring (locale_id));

					lpnmdi->lpszText = buffer;

					break;
				}

				case LVN_ITEMCHANGED:
				{
					LPNMLISTVIEW lpnmlv;
					PITEM_COLOR ptr_clr;
					INT listview_id;
					BOOLEAN is_enabled;

					lpnmlv = (LPNMLISTVIEW)lparam;
					listview_id = (INT)(INT_PTR)lpnmlv->hdr.idFrom;

					if (listview_id != IDC_COLORS)
						break;

					if ((lpnmlv->uChanged & LVIF_STATE) == 0)
						break;

					if ((lpnmlv->uNewState & LVIS_STATEIMAGEMASK) == INDEXTOSTATEIMAGEMASK (1) || ((lpnmlv->uNewState & LVIS_STATEIMAGEMASK) == INDEXTOSTATEIMAGEMASK (2)))
					{
						if (_app_listview_islocked (hwnd, (INT)(INT_PTR)lpnmlv->hdr.idFrom))
							break;

						ptr_clr = (PITEM_COLOR)lpnmlv->lParam;

						if (ptr_clr)
						{
							is_enabled = (lpnmlv->uNewState & LVIS_STATEIMAGEMASK) == INDEXTOSTATEIMAGEMASK (2);

							_r_config_setboolean_ex (ptr_clr->config_name->buffer, is_enabled, L"colors");

							_r_listview_redraw (_r_app_gethwnd (), _app_listview_getcurrent (_r_app_gethwnd ()));
						}
					}

					break;
				}

				case NM_CUSTOMDRAW:
				{
					LONG_PTR result;

					result = _app_message_custdraw (hwnd, (LPNMLVCUSTOMDRAW)lparam);

					SetWindowLongPtrW (hwnd, DWLP_MSGRESULT, result);

					return result;
				}

				case NM_DBLCLK:
				{
					LPNMITEMACTIVATE lpnmlv;
					PITEM_COLOR ptr_clr_crnt;
					PITEM_COLOR ptr_clr = NULL;
					CHOOSECOLOR cc = {0};
					COLORREF cust[16] = {0};
					ULONG_PTR enum_key = 0;
					ULONG_PTR index = 0;
					INT listview_id;

					lpnmlv = (LPNMITEMACTIVATE)lparam;
					listview_id = (INT)(INT_PTR)lpnmlv->hdr.idFrom;

					if (lpnmlv->iItem == INT_ERROR)
						break;

					if (listview_id != IDC_COLORS)
						break;

					ptr_clr_crnt = (PITEM_COLOR)_r_listview_getitemlparam (hwnd, listview_id, lpnmlv->iItem);

					if (!ptr_clr_crnt)
						break;

					while (_r_obj_enumhashtable (colors_table, &ptr_clr, NULL, &enum_key))
					{
						cust[index++] = ptr_clr->default_clr;
					}

					cc.lStructSize = sizeof (cc);
					cc.Flags = CC_RGBINIT | CC_FULLOPEN;
					cc.hwndOwner = hwnd;
					cc.lpCustColors = cust;
					cc.rgbResult = ptr_clr_crnt->new_clr;

					if (ChooseColorW (&cc))
					{
						ptr_clr_crnt->new_clr = cc.rgbResult;

						_r_config_setulong_ex (ptr_clr_crnt->config_value->buffer, cc.rgbResult, L"colors");

						_r_listview_redraw (hwnd, IDC_COLORS);

						_r_listview_redraw (_r_app_gethwnd (), _app_listview_getcurrent (_r_app_gethwnd ()));
					}

					break;
				}

				case NM_CLICK:
				case NM_RETURN:
				{
					PNMLINK pnmlink;

					pnmlink = (PNMLINK)lparam;

					if (!_r_str_isempty (pnmlink->item.szUrl))
						_r_shell_opendefault (pnmlink->item.szUrl);

					break;
				}
			}

			break;
		}

		case WM_COMMAND:
		{
			INT ctrl_id = LOWORD (wparam);
			INT notify_code = HIWORD (wparam);

			switch (ctrl_id)
			{
				case IDC_ALWAYSONTOP_CHK:
				{
					BOOLEAN is_enabled;

					is_enabled = _r_ctrl_isbuttonchecked (hwnd, ctrl_id);

					_r_config_setboolean (L"AlwaysOnTop", is_enabled);
					_r_menu_checkitem (GetMenu (_r_app_gethwnd ()), IDM_ALWAYSONTOP_CHK, 0, MF_BYCOMMAND, is_enabled);

					break;
				}

				case IDC_LANGUAGE:
				{
					if (notify_code != CBN_SELCHANGE)
						break;

					_r_locale_apply (hwnd, ctrl_id, 0);

					break;
				}

				case IDC_CONFIRMEXIT_CHK:
				{
					_r_config_setboolean (L"ConfirmExit2", _r_ctrl_isbuttonchecked (hwnd, ctrl_id));
					break;
				}

				case IDC_CONFIRMEXITTIMER_CHK:
				{
					_r_config_setboolean (L"ConfirmExitTimer", _r_ctrl_isbuttonchecked (hwnd, ctrl_id));
					break;
				}

				case IDC_CONFIRMLOGCLEAR_CHK:
				{
					_r_config_setboolean (L"ConfirmLogClear", _r_ctrl_isbuttonchecked (hwnd, ctrl_id));
					break;
				}

				case IDC_CONFIRMALLOW_CHK:
				{
					_r_config_setboolean (L"ConfirmAllow", _r_ctrl_isbuttonchecked (hwnd, ctrl_id));
					break;
				}

				case IDC_TRAYICONSINGLECLICK_CHK:
				{
					_r_config_setboolean (L"IsTrayIconSingleClick", _r_ctrl_isbuttonchecked (hwnd, ctrl_id));
					break;
				}

				case IDC_LOADONSTARTUP_CHK:
				case IDC_STARTMINIMIZED_CHK:
				case IDC_SKIPUACWARNING_CHK:
				case IDC_CHECKUPDATES_CHK:
				case IDC_RULE_BLOCKOUTBOUND:
				case IDC_RULE_BLOCKINBOUND:
				case IDC_RULE_ALLOWLOOPBACK:
				case IDC_RULE_ALLOW6TO4:
				case IDC_USESTEALTHMODE_CHK:
				case IDC_INSTALLBOOTTIMEFILTERS_CHK:
				case IDC_USECERTIFICATES_CHK:
				case IDC_USEHASHES_CHK:
				case IDC_USENETWORKRESOLUTION_CHK:
				{
					_app_config_apply (_r_app_gethwnd (), hwnd, ctrl_id);
					break;
				}

				case IDC_BLOCKLIST_SPY_DISABLE:
				case IDC_BLOCKLIST_SPY_ALLOW:
				case IDC_BLOCKLIST_SPY_BLOCK:
				case IDC_BLOCKLIST_UPDATE_DISABLE:
				case IDC_BLOCKLIST_UPDATE_ALLOW:
				case IDC_BLOCKLIST_UPDATE_BLOCK:
				case IDC_BLOCKLIST_EXTRA_DISABLE:
				case IDC_BLOCKLIST_EXTRA_ALLOW:
				case IDC_BLOCKLIST_EXTRA_BLOCK:
				{
					HMENU hmenu;
					LONG new_state;

					hmenu = GetMenu (_r_app_gethwnd ());

					if (ctrl_id >= IDC_BLOCKLIST_SPY_DISABLE && ctrl_id <= IDC_BLOCKLIST_SPY_BLOCK)
					{
						new_state = _r_calc_clamp (
							_r_ctrl_isradiochecked (hwnd, IDC_BLOCKLIST_SPY_DISABLE, IDC_BLOCKLIST_SPY_BLOCK) - IDC_BLOCKLIST_SPY_DISABLE,
							0,
							2
						);

						_r_menu_checkitem (hmenu, IDM_BLOCKLIST_SPY_DISABLE, IDM_BLOCKLIST_SPY_BLOCK, MF_BYCOMMAND, IDM_BLOCKLIST_SPY_DISABLE + new_state);

						_r_config_setlong (L"BlocklistSpyState", new_state);

						_app_ruleblocklistset (_r_app_gethwnd (), new_state, INT_ERROR, INT_ERROR, TRUE);
					}
					else if (ctrl_id >= IDC_BLOCKLIST_UPDATE_DISABLE && ctrl_id <= IDC_BLOCKLIST_UPDATE_BLOCK)
					{
						new_state = _r_calc_clamp (
							_r_ctrl_isradiochecked (hwnd, IDC_BLOCKLIST_UPDATE_DISABLE, IDC_BLOCKLIST_UPDATE_BLOCK) - IDC_BLOCKLIST_UPDATE_DISABLE,
							0,
							2
						);

						_r_menu_checkitem (hmenu, IDM_BLOCKLIST_UPDATE_DISABLE, IDM_BLOCKLIST_UPDATE_BLOCK, MF_BYCOMMAND, IDM_BLOCKLIST_UPDATE_DISABLE + new_state);

						_r_config_setlong (L"BlocklistUpdateState", new_state);

						_app_ruleblocklistset (_r_app_gethwnd (), INT_ERROR, new_state, INT_ERROR, TRUE);
					}
					else if (ctrl_id >= IDC_BLOCKLIST_EXTRA_DISABLE && ctrl_id <= IDC_BLOCKLIST_EXTRA_BLOCK)
					{
						new_state = _r_calc_clamp (
							_r_ctrl_isradiochecked (hwnd, IDC_BLOCKLIST_EXTRA_DISABLE, IDC_BLOCKLIST_EXTRA_BLOCK) - IDC_BLOCKLIST_EXTRA_DISABLE,
							0,
							2
						);

						_r_menu_checkitem (hmenu, IDM_BLOCKLIST_EXTRA_DISABLE, IDM_BLOCKLIST_EXTRA_BLOCK, MF_BYCOMMAND, IDM_BLOCKLIST_EXTRA_DISABLE + new_state);

						_r_config_setlong (L"BlocklistExtraState", new_state);

						_app_ruleblocklistset (_r_app_gethwnd (), INT_ERROR, INT_ERROR, new_state, TRUE);
					}

					break;
				}

				case IDC_ENABLELOG_CHK:
				{
					HWND hctrl;
					BOOLEAN is_postmessage;
					BOOLEAN is_enabled;
					BOOLEAN is_logging_enabled;

					is_postmessage = ((INT)lparam == WM_APP);
					is_enabled = _r_ctrl_isbuttonchecked (hwnd, ctrl_id);
					is_logging_enabled = is_enabled || _r_ctrl_isbuttonchecked (hwnd, IDC_ENABLEUILOG_CHK);

					if (!is_postmessage)
					{
						_r_config_setboolean (L"IsLogEnabled", is_enabled);

						_app_loginit (is_enabled);
					}

					_r_toolbar_pressbutton (config.hrebar, IDC_TOOLBAR, IDM_TRAY_ENABLELOG_CHK, is_enabled);

					_r_ctrl_enable (hwnd, IDC_LOGPATH, is_enabled); // input
					_r_ctrl_enable (hwnd, IDC_LOGPATH_BTN, is_enabled); // button

					hctrl = _r_updown_getbuddy (hwnd, IDC_LOGSIZELIMIT);

					if (hctrl)
						_r_ctrl_enable (hctrl, 0, is_enabled);

					break;
				}

				case IDC_ENABLEUILOG_CHK:
				{
					BOOLEAN is_enabled;

					is_enabled = _r_ctrl_isbuttonchecked (hwnd, ctrl_id);

					_r_config_setboolean (L"IsLogUiEnabled", is_enabled);

					_r_toolbar_pressbutton (config.hrebar, IDC_TOOLBAR, IDM_TRAY_ENABLEUILOG_CHK, is_enabled);

					break;
				}

				case IDC_LOGPATH:
				{
					PR_STRING log_path;

					if (notify_code == EN_KILLFOCUS)
					{
						log_path = _r_ctrl_getstring (hwnd, ctrl_id);

						if (log_path)
						{
							_r_config_setstringexpand (L"LogPath", log_path->buffer);

							if (_r_config_getboolean (L"IsLogEnabled", FALSE))
								_app_loginit (TRUE);

							_r_obj_dereference (log_path);
						}
					}

					break;
				}

				case IDC_LOGPATH_BTN:
				{
					static COMDLG_FILTERSPEC filters[] = {
						L"Log files (*.log, *.csv)", L"*.log;*.csv",
						L"All files (*.*)", L"*.*",
					};

					R_FILE_DIALOG file_dialog;
					PR_STRING path;
					HRESULT status;

					status = _r_filedialog_initialize (&file_dialog, PR_FILEDIALOG_SAVEFILE);

					if (SUCCEEDED (status))
					{
						_r_filedialog_setfilter (&file_dialog, filters, RTL_NUMBER_OF (filters));

						path = _r_ctrl_getstring (hwnd, IDC_LOGPATH);

						if (path)
						{
							_r_filedialog_setpath (&file_dialog, path->buffer);

							_r_obj_dereference (path);
						}

						status = _r_filedialog_show (hwnd, &file_dialog);

						if (SUCCEEDED (status))
						{
							status = _r_filedialog_getpath (&file_dialog, &path);

							if (SUCCEEDED (status))
							{
								_r_config_setstringexpand (L"LogPath", path->buffer);
								_r_ctrl_setstring (hwnd, IDC_LOGPATH, path->buffer);

								_app_loginit (_r_config_getboolean (L"IsLogEnabled", FALSE));

								_r_obj_dereference (path);
							}
						}

						_r_filedialog_destroy (&file_dialog);
					}

					break;
				}

				case IDC_LOGVIEWER:
				{
					PR_STRING log_viewer;

					if (notify_code != EN_KILLFOCUS)
						break;

					log_viewer = _r_ctrl_getstring (hwnd, ctrl_id);

					if (!log_viewer)
						break;

					_r_config_setstringexpand (L"LogViewer", log_viewer->buffer);

					_r_obj_dereference (log_viewer);

					break;
				}

				case IDC_LOGVIEWER_BTN:
				{
					static COMDLG_FILTERSPEC filters[] = {
						L"Executable files (*.exe)", L"*.exe",
						L"All files (*.*)", L"*.*",
					};

					R_FILE_DIALOG file_dialog;
					PR_STRING path;
					HRESULT status;

					status = _r_filedialog_initialize (&file_dialog, PR_FILEDIALOG_OPENFILE);

					if (SUCCEEDED (status))
					{
						_r_filedialog_setfilter (&file_dialog, filters, RTL_NUMBER_OF (filters));

						path = _r_ctrl_getstring (hwnd, IDC_LOGVIEWER);

						if (path)
						{
							_r_filedialog_setpath (&file_dialog, path->buffer);

							_r_obj_dereference (path);
						}

						status = _r_filedialog_show (hwnd, &file_dialog);

						if (SUCCEEDED (status))
						{
							status = _r_filedialog_getpath (&file_dialog, &path);

							if (SUCCEEDED (status))
							{
								_r_config_setstringexpand (L"LogViewer", path->buffer);
								_r_ctrl_setstring (hwnd, IDC_LOGVIEWER, path->buffer);

								_r_obj_dereference (path);
							}
						}

						_r_filedialog_destroy (&file_dialog);
					}

					break;
				}

				case IDC_LOGSIZELIMIT_CTRL:
				{
					LONG value;

					if (notify_code != EN_KILLFOCUS)
						break;

					value = _r_updown_getvalue (hwnd, IDC_LOGSIZELIMIT);

					_r_config_setlong (L"LogSizeLimitKb", value);

					break;
				}

				case IDC_ENABLENOTIFICATIONS_CHK:
				{
					HWND hctrl;
					BOOLEAN is_postmessage;
					BOOLEAN is_enabled;

					is_postmessage = ((INT)lparam == WM_APP);
					is_enabled = _r_ctrl_isbuttonchecked (hwnd, ctrl_id);

					if (!is_postmessage)
						_r_config_setboolean (L"IsNotificationsEnabled", is_enabled);

					_r_toolbar_pressbutton (config.hrebar, IDC_TOOLBAR, IDM_TRAY_ENABLENOTIFICATIONS_CHK, is_enabled);

					_r_ctrl_enable (hwnd, IDC_NOTIFICATIONSOUND_CHK, is_enabled);
					_r_ctrl_enable (hwnd, IDC_NOTIFICATIONONTRAY_CHK, is_enabled);

					hctrl = _r_updown_getbuddy (hwnd, IDC_NOTIFICATIONTIMEOUT);

					if (hctrl)
						_r_ctrl_enable (hctrl, 0, is_enabled);

					_r_ctrl_sendcommand (hwnd, IDC_NOTIFICATIONSOUND_CHK, WM_APP);

					if (is_postmessage)
						break;

					hctrl = _app_notify_getwindow (NULL);

					if (hctrl)
						_app_notify_refresh (hctrl);

					break;
				}

				case IDC_NOTIFICATIONSOUND_CHK:
				{
					BOOLEAN is_postmessage;
					BOOLEAN is_checked;

					is_postmessage = ((INT)lparam == WM_APP);
					is_checked = _r_ctrl_isbuttonchecked (hwnd, ctrl_id);

					_r_ctrl_checkbutton (hwnd, IDC_NOTIFICATIONFULLSCREENSILENTMODE_CHK, _r_config_getboolean (L"IsNotificationsFullscreenSilentMode", TRUE));

					if (!is_postmessage)
						_r_config_setboolean (L"IsNotificationsSound", is_checked);

					_r_ctrl_enable (hwnd, IDC_NOTIFICATIONFULLSCREENSILENTMODE_CHK, _r_ctrl_isenabled (hwnd, ctrl_id) && is_checked);

					break;
				}

				case IDC_NOTIFICATIONFULLSCREENSILENTMODE_CHK:
				{
					_r_config_setboolean (L"IsNotificationsFullscreenSilentMode", _r_ctrl_isbuttonchecked (hwnd, ctrl_id));
					break;
				}

				case IDC_NOTIFICATIONONTRAY_CHK:
				{
					HWND hnotify;

					_r_config_setboolean (L"IsNotificationsOnTray", _r_ctrl_isbuttonchecked (hwnd, ctrl_id));

					hnotify = _app_notify_getwindow (NULL);

					if (hnotify)
						_app_notify_setposition (hnotify, TRUE);

					break;
				}

				case IDC_NOTIFICATIONTIMEOUT_CTRL:
				{
					if (notify_code == EN_KILLFOCUS)
						_r_config_setlong (L"NotificationsTimeout", _r_updown_getvalue (hwnd, IDC_NOTIFICATIONTIMEOUT));

					break;
				}

				case IDC_EXCLUDESTEALTH_CHK:
				{
					_r_config_setboolean (L"IsExcludeStealth", _r_ctrl_isbuttonchecked (hwnd, ctrl_id));
					break;
				}

				case IDC_EXCLUDECLASSIFYALLOW_CHK:
				{
					HANDLE hengine;

					_r_config_setboolean (L"IsExcludeClassifyAllow", _r_ctrl_isbuttonchecked (hwnd, ctrl_id));

					hengine = _wfp_getenginehandle ();

					_wfp_logsetoption (hengine);

					break;
				}

				case IDC_EXCLUDEBLOCKLIST_CHK:
				{
					_r_config_setboolean (L"IsExcludeBlocklist", _r_ctrl_isbuttonchecked (hwnd, ctrl_id));
					break;
				}

				case IDC_EXCLUDECUSTOM_CHK:
				{
					_r_config_setboolean (L"IsExcludeCustomRules", _r_ctrl_isbuttonchecked (hwnd, ctrl_id));
					break;
				}
			}

			break;
		}
	}

	return FALSE;
}

VOID _app_addwindowtabs (
	_In_ HWND hwnd
)
{
	PITEM_TAB_CONTEXT context;

	// apps profile tab
	context = _r_mem_allocate (sizeof (ITEM_TAB_CONTEXT));

	context->listview_id = IDC_APPS_PROFILE;
	context->locale_id = IDS_TAB_APPS;

	_r_tab_additem (hwnd, IDC_TAB, INT_MAX, L"", I_DEFAULT, (LPARAM)context);

	// apps services tab
	context = _r_mem_allocate (sizeof (ITEM_TAB_CONTEXT));

	context->listview_id = IDC_APPS_SERVICE;
	context->locale_id = IDS_TAB_SERVICES;

	_r_tab_additem (hwnd, IDC_TAB, INT_MAX, L"", I_DEFAULT, (LPARAM)context);

	// uwp apps (win8+)
	if (_r_sys_isosversiongreaterorequal (WINDOWS_8))
	{
		context = _r_mem_allocate (sizeof (ITEM_TAB_CONTEXT));

		context->listview_id = IDC_APPS_UWP;
		context->locale_id = IDS_TAB_PACKAGES;

		_r_tab_additem (hwnd, IDC_TAB, INT_MAX, L"", I_DEFAULT, (LPARAM)context);
	}

	if (!_r_config_getboolean (L"IsInternalRulesDisabled", FALSE))
	{
		// rules blocklist tab
		context = _r_mem_allocate (sizeof (ITEM_TAB_CONTEXT));

		context->listview_id = IDC_RULES_BLOCKLIST;
		context->locale_id = IDS_TRAY_BLOCKLIST_RULES;

		_r_tab_additem (hwnd, IDC_TAB, INT_MAX, L"", I_DEFAULT, (LPARAM)context);

		// rules system tab
		context = _r_mem_allocate (sizeof (ITEM_TAB_CONTEXT));

		context->listview_id = IDC_RULES_SYSTEM;
		context->locale_id = IDS_TRAY_SYSTEM_RULES;

		_r_tab_additem (hwnd, IDC_TAB, INT_MAX, L"", I_DEFAULT, (LPARAM)context);
	}

	// rules user tab
	context = _r_mem_allocate (sizeof (ITEM_TAB_CONTEXT));

	context->listview_id = IDC_RULES_CUSTOM;
	context->locale_id = IDS_TRAY_USER_RULES;

	_r_tab_additem (hwnd, IDC_TAB, INT_MAX, L"", I_DEFAULT, (LPARAM)context);

	// network tab
	context = _r_mem_allocate (sizeof (ITEM_TAB_CONTEXT));

	context->listview_id = IDC_NETWORK;
	context->locale_id = IDS_TAB_NETWORK;

	// log tab
	_r_tab_additem (hwnd, IDC_TAB, INT_MAX, L"", I_DEFAULT, (LPARAM)context);

	context = _r_mem_allocate (sizeof (ITEM_TAB_CONTEXT));

	context->listview_id = IDC_LOG;
	context->locale_id = IDS_TITLE_LOGGING;

	_r_tab_additem (hwnd, IDC_TAB, INT_MAX, L"", I_DEFAULT, (LPARAM)context);

	_r_tab_additem (hwnd, IDC_TAB, INT_MAX, L"", I_DEFAULT, (LPARAM)IDC_LOG);
}

VOID _app_tabs_init (
	_In_ HWND hwnd,
	_In_ LONG dpi_value
)
{
	PITEM_TAB_CONTEXT context;
	RECT rect = {0};
	LONG statusbar_height;
	LONG rebar_height;
	ULONG style;
	HWND hlistview;

	statusbar_height = _r_status_getheight (hwnd, IDC_STATUSBAR);
	rebar_height = _r_rebar_getheight (hwnd, IDC_REBAR);

	GetClientRect (hwnd, &rect);

	SetWindowPos (
		config.hrebar,
		NULL,
		0,
		0,
		rect.right,
		rebar_height,
		SWP_NOZORDER | SWP_NOACTIVATE | SWP_NOOWNERZORDER
	);

	SetWindowPos (
		GetDlgItem (hwnd, IDC_TAB),
		NULL
		,
		0,
		rebar_height,
		rect.right,
		rect.bottom - rebar_height - statusbar_height,
		SWP_NOZORDER | SWP_NOACTIVATE | SWP_NOOWNERZORDER
	);

	_app_listview_loadfont (dpi_value, TRUE);

	style = LVS_EX_DOUBLEBUFFER | LVS_EX_FULLROWSELECT | LVS_EX_INFOTIP | LVS_EX_LABELTIP | LVS_EX_HEADERINALLVIEWS | LVS_EX_HEADERDRAGDROP;

	_app_addwindowtabs (hwnd);

	for (INT i = 0; i < _r_tab_getitemcount (hwnd, IDC_TAB) - 1; i++)
	{
		context = _app_listview_getbytab (hwnd, i);

		hlistview = GetDlgItem (hwnd, context->listview_id);

		if (!hlistview)
			continue;

		_r_tab_adjustchild (hwnd, IDC_TAB, hlistview);

		if (context->listview_id >= IDC_APPS_PROFILE && context->listview_id <= IDC_RULES_CUSTOM)
		{
			_r_listview_setstyle (hwnd, context->listview_id, style | LVS_EX_CHECKBOXES, TRUE); // with checkboxes

			if (context->listview_id >= IDC_APPS_PROFILE && context->listview_id <= IDC_APPS_UWP)
			{
				_r_listview_addcolumn (hwnd, context->listview_id, 0, L"", -80, LVCFMT_LEFT);
				_r_listview_addcolumn (hwnd, context->listview_id, 1, L"", -20, LVCFMT_RIGHT);
			}
			else
			{
				_r_listview_addcolumn (hwnd, context->listview_id, 0, L"", -80, LVCFMT_LEFT);
				_r_listview_addcolumn (hwnd, context->listview_id, 1, L"", -10, LVCFMT_RIGHT);
				_r_listview_addcolumn (hwnd, context->listview_id, 2, L"", -10, LVCFMT_RIGHT);
			}

			_r_listview_addgroup (hwnd, context->listview_id, 0, L"", 0, LVGS_COLLAPSIBLE, LVGS_COLLAPSIBLE);
			_r_listview_addgroup (hwnd, context->listview_id, 1, L"", 0, LVGS_COLLAPSIBLE, LVGS_COLLAPSIBLE);
			_r_listview_addgroup (hwnd, context->listview_id, 2, L"", 0, LVGS_COLLAPSIBLE, LVGS_COLLAPSIBLE);
			_r_listview_addgroup (hwnd, context->listview_id, 3, L"", 0, LVGS_COLLAPSIBLE, LVGS_COLLAPSIBLE);
			_r_listview_addgroup (hwnd, context->listview_id, 4, L"", 0, LVGS_COLLAPSIBLE, LVGS_COLLAPSIBLE);
		}
		else if (context->listview_id == IDC_NETWORK)
		{
			_r_listview_setstyle (hwnd, context->listview_id, style, TRUE);

			_r_listview_addcolumn (hwnd, context->listview_id, 0, L"", -20, LVCFMT_LEFT);
			_r_listview_addcolumn (hwnd, context->listview_id, 1, L"", -10, LVCFMT_LEFT);
			_r_listview_addcolumn (hwnd, context->listview_id, 2, L"", -10, LVCFMT_LEFT);
			_r_listview_addcolumn (hwnd, context->listview_id, 3, L"", -10, LVCFMT_RIGHT);
			_r_listview_addcolumn (hwnd, context->listview_id, 4, L"", -10, LVCFMT_LEFT);
			_r_listview_addcolumn (hwnd, context->listview_id, 5, L"", -10, LVCFMT_LEFT);
			_r_listview_addcolumn (hwnd, context->listview_id, 6, L"", -10, LVCFMT_RIGHT);
			_r_listview_addcolumn (hwnd, context->listview_id, 7, L"", -10, LVCFMT_RIGHT);
			_r_listview_addcolumn (hwnd, context->listview_id, 8, L"", -10, LVCFMT_RIGHT);

			_r_listview_addgroup (hwnd, context->listview_id, 0, L"", 0, LVGS_COLLAPSIBLE, LVGS_COLLAPSIBLE);
			_r_listview_addgroup (hwnd, context->listview_id, 1, L"", 0, LVGS_COLLAPSIBLE, LVGS_COLLAPSIBLE);
			_r_listview_addgroup (hwnd, context->listview_id, 2, L"", 0, LVGS_COLLAPSIBLE, LVGS_COLLAPSIBLE);
		}
		else if (context->listview_id == IDC_LOG)
		{
			_r_listview_setstyle (hwnd, context->listview_id, style, TRUE);

			_r_listview_addcolumn (hwnd, context->listview_id, 0, L"", -10, LVCFMT_LEFT);
			_r_listview_addcolumn (hwnd, context->listview_id, 1, L"", -10, LVCFMT_LEFT);
			_r_listview_addcolumn (hwnd, context->listview_id, 2, L"", -10, LVCFMT_LEFT);
			_r_listview_addcolumn (hwnd, context->listview_id, 3, L"", -10, LVCFMT_LEFT);
			_r_listview_addcolumn (hwnd, context->listview_id, 4, L"", -10, LVCFMT_RIGHT);
			_r_listview_addcolumn (hwnd, context->listview_id, 5, L"", -10, LVCFMT_LEFT);
			_r_listview_addcolumn (hwnd, context->listview_id, 6, L"", -10, LVCFMT_LEFT);
			_r_listview_addcolumn (hwnd, context->listview_id, 7, L"", -10, LVCFMT_RIGHT);
			_r_listview_addcolumn (hwnd, context->listview_id, 8, L"", -10, LVCFMT_LEFT);
			_r_listview_addcolumn (hwnd, context->listview_id, 9, L"", -10, LVCFMT_RIGHT);
			_r_listview_addcolumn (hwnd, context->listview_id, 10, L"", -10, LVCFMT_LEFT);

			_r_listview_addgroup (hwnd, context->listview_id, 0, L"", 0, LVGS_NOHEADER, LVGS_NOHEADER);
		}

		// add filter group
		_r_listview_addgroup (hwnd, context->listview_id, LV_HIDDEN_GROUP_ID, L"", 0, LVGS_HIDDEN | LVGS_NOHEADER | LVGS_COLLAPSED, LVGS_HIDDEN | LVGS_NOHEADER | LVGS_COLLAPSED);

		_app_listview_setfont (hwnd, context->listview_id);

		BringWindowToTop (hlistview); // HACK!!!
	}
}

VOID _app_initialize (
	_In_opt_ HWND hwnd
)
{
	ULONG privileges[] = {
		SE_SECURITY_PRIVILEGE,
		SE_TAKE_OWNERSHIP_PRIVILEGE,
		SE_INC_BASE_PRIORITY_PRIVILEGE,
		SE_BACKUP_PRIVILEGE,
		SE_RESTORE_PRIVILEGE,
		SE_DEBUG_PRIVILEGE,
	};

	R_ENVIRONMENT environment;

	// set privileges
	_r_sys_setprocessprivilege (NtCurrentProcess (), privileges, RTL_NUMBER_OF (privileges), TRUE);

	// set process priority
	_r_sys_setenvironment (&environment, PROCESS_PRIORITY_CLASS_HIGH, IoPriorityHigh, MEMORY_PRIORITY_NORMAL);

	_r_sys_setprocessenvironment (NtCurrentProcess (), &environment);

	// initialize workqueue
	_r_sys_setenvironment (&environment, THREAD_PRIORITY_ABOVE_NORMAL, IoPriorityHigh, MEMORY_PRIORITY_NORMAL);

	_r_workqueue_initialize (&file_queue, 12, &environment, L"FilesQueue");
	_r_workqueue_initialize (&resolve_notify_queue, 2, &environment, L"NotificationQueue");

	_r_sys_setenvironment (&environment, THREAD_PRIORITY_BELOW_NORMAL, IoPriorityLow, MEMORY_PRIORITY_NORMAL);

	_r_workqueue_initialize (&log_queue, 6, &environment, L"PacketsQueue");
	_r_workqueue_initialize (&resolver_queue, 6, &environment, L"ResolverQueue");

	_r_sys_setenvironment (&environment, THREAD_PRIORITY_HIGHEST, IoPriorityHigh, MEMORY_PRIORITY_NORMAL);

	_r_workqueue_initialize (&wfp_queue, 1, &environment, L"FiltersQueue");

	// static initializer
	_r_sys_getsystemroot (&config.windows_dir);

	_app_profile_initialize ();

	config.my_path = _r_obj_createstring3 (&NtCurrentPeb ()->ProcessParameters->ImagePathName);

	config.svchost_path = _r_obj_concatstrings (
		2,
		_r_sys_getsystemdirectory ()->buffer,
		PATH_SVCHOST
	);

	config.wusvc_path = _r_obj_concatstrings (
		2,
		_r_sys_getsystemdirectory ()->buffer,
		PATH_WUSVC
	);

	config.system_path = _r_obj_createstring (PROC_SYSTEM_NAME);

	config.ntoskrnl_path = _r_sys_getkernelfilename (TRUE);

	if (!config.ntoskrnl_path)
		config.ntoskrnl_path = _r_obj_referenceemptystring ();

	config.my_hash = _r_str_gethash2 (&config.my_path->sr, TRUE);
	config.ntoskrnl_hash = _r_str_gethash2 (&config.system_path->sr, TRUE);
	config.svchost_hash = _r_str_gethash2 (&config.svchost_path->sr, TRUE);
	config.wusvc_hash = _r_str_gethash2 (&config.wusvc_path->sr, TRUE);

	// initialize free list
	_r_freelist_initialize (&context_free_list, sizeof (ITEM_CONTEXT), 32);
	_r_freelist_initialize (&listview_free_list, sizeof (ITEM_LISTVIEW_CONTEXT), 2048);

	// initialize colors array
	colors_table = _r_obj_createhashtable (sizeof (ITEM_COLOR), 8, NULL);

	// initialize colors
	config.color_invalid = _app_addcolor (IDS_HIGHLIGHT_INVALID, L"IsHighlightInvalid", TRUE, L"ColorInvalid", LV_COLOR_INVALID);
	config.color_special = _app_addcolor (IDS_HIGHLIGHT_SPECIAL, L"IsHighlightSpecial", TRUE, L"ColorSpecial", LV_COLOR_SPECIAL);
	config.color_signed = _app_addcolor (IDS_HIGHLIGHT_SIGNED, L"IsHighlightSigned", TRUE, L"ColorSigned", LV_COLOR_SIGNED);
	config.color_pico = _app_addcolor (IDS_HIGHLIGHT_PICO, L"IsHighlightPico", TRUE, L"ColorPico", LV_COLOR_PICO);
	config.color_system = _app_addcolor (IDS_HIGHLIGHT_SYSTEM, L"IsHighlightSystem", TRUE, L"ColorSystem", LV_COLOR_SYSTEM);
	config.color_network = _app_addcolor (IDS_HIGHLIGHT_CONNECTION, L"IsHighlightConnection", TRUE, L"ColorConnection", LV_COLOR_CONNECTION);
	config.color_nonremovable = _app_addcolor (IDS_DISABLEREMOVAL, L"IsHighlightUndelete", TRUE, L"ColorUndelete", LV_COLOR_UNDELETE);

	_app_generate_credentials ();

	// load default icons
	_app_icons_getdefault ();

	// initialize global filters array object
	filter_ids = _r_obj_createarray (sizeof (GUID), 10, NULL);

	// initialize apps table
	apps_table = _r_obj_createhashtablepointer (32);

	// initialize rules array object
	rules_list = _r_obj_createlist (10, &_r_obj_dereference);

	// initialize rules configuration table
	rules_config = _r_obj_createhashtable (sizeof (ITEM_RULE_CONFIG), 8, &_app_dereferenceruleconfig);

	// initialize log hashtable object
	log_table = _r_obj_createhashtablepointer (32);

	// initialize cache table
	cache_information = _r_obj_createhashtablepointer (32);

	cache_resolution = _r_obj_createhashtablepointer (32);

	NtCreateEvent (&config.hnotify_evt, EVENT_ALL_ACCESS, NULL, NotificationEvent, TRUE);

	if (hwnd)
		_r_sys_createthread (NULL, NtCurrentProcess (), &_app_package_threadproc, hwnd, &environment, L"ServicesMonitor");
}

INT FirstDriveFromMask (
	_In_ ULONG unitmask
)
{
	INT i;

	for (i = 0; i < PR_DEVICE_COUNT; ++i)
	{
		if (unitmask & 0x1)
			break;

		unitmask = unitmask >> 1;
	}

	return i;
}

INT_PTR CALLBACK DlgProc (
	_In_ HWND hwnd,
	_In_ UINT msg,
	_In_ WPARAM wparam,
	_In_ LPARAM lparam
)
{
	switch (msg)
	{
		case WM_INITDIALOG:
		{
			WCHAR internal_profile_version[64];
			ENUM_INSTALL_TYPE install_type;
			LONG dpi_value;

			// initialize vars
			dpi_value = _r_dc_getwindowdpi (hwnd);

			_r_app_sethwnd (hwnd); // HACK!!!

			_app_initialize (hwnd);

			// init buffered paint
			BufferedPaintInit ();

			// allow drag&drop support
			DragAcceptFiles (hwnd, TRUE);

			// restore window position (required!)
			_r_window_restoreposition (hwnd, L"window");

			// initialize imagelist
			_app_imagelist_init (hwnd, dpi_value);

			// initialize toolbar
			_app_toolbar_init (hwnd, dpi_value);
			_app_toolbar_resize (hwnd, dpi_value);

			// initialize tabs
			_app_tabs_init (hwnd, dpi_value);

			// load profile
			_app_profile_load (hwnd, NULL);

			// install filters
			install_type = _wfp_isfiltersinstalled ();

			if (install_type != INSTALL_DISABLED)
			{
				if (install_type == INSTALL_ENABLED_TEMPORARY)
					config.is_filterstemporary = TRUE;

				_app_changefilters (hwnd, TRUE, TRUE);
			}
			else
			{
				_r_status_settext (hwnd, IDC_STATUSBAR, 0, _app_getstatelocale (install_type));
			}

			// initialize settings
			_r_settings_addpage (IDD_SETTINGS_GENERAL, IDS_SETTINGS_GENERAL);
			_r_settings_addpage (IDD_SETTINGS_INTERFACE, IDS_TITLE_INTERFACE);
			_r_settings_addpage (IDD_SETTINGS_HIGHLIGHTING, IDS_TITLE_HIGHLIGHTING);
			_r_settings_addpage (IDD_SETTINGS_RULES, IDS_TRAY_RULES);

			if (!_r_config_getboolean (L"IsInternalRulesDisabled", FALSE))
				_r_settings_addpage (IDD_SETTINGS_BLOCKLIST, IDS_TRAY_BLOCKLIST_RULES);

			// dropped packets logging (win7+)
			_r_settings_addpage (IDD_SETTINGS_NOTIFICATIONS, IDS_TITLE_NOTIFICATIONS);
			_r_settings_addpage (IDD_SETTINGS_LOGGING, IDS_TITLE_LOGGING);
			_r_settings_addpage (IDD_SETTINGS_EXCLUDE, IDS_TITLE_EXCLUDE);

			// add blocklist to update
			if (!_r_config_getboolean (L"IsInternalRulesDisabled", FALSE))
			{
				_r_str_fromlong64 (internal_profile_version, RTL_NUMBER_OF (internal_profile_version), profile_info.profile_internal_timestamp);

				_r_update_addcomponent (L"Internal rules", L"rules_internal", internal_profile_version, profile_info.profile_path_internal, FALSE);
			}

			_app_network_initialize (hwnd);

			// initialize tab
			_app_settab_id (hwnd, _r_config_getlong (L"CurrentTab", IDC_APPS_PROFILE));

			_app_fileloggingenable ();

			break;
		}

		case RM_INITIALIZE:
		{
			_app_message_initialize (hwnd);
			break;
		}

		case RM_UNINITIALIZE:
		{
			_app_message_uninitialize (hwnd);
			break;
		}

		case RM_LOCALIZE:
		{
			_app_message_localize (hwnd);
			break;
		}

		case RM_TASKBARCREATED:
		{
			ENUM_INSTALL_TYPE install_type;

			install_type = _wfp_getinstalltype ();

			// refresh tray icon
			_r_tray_create (hwnd, &GUID_TrayIcon, RM_TRAYICON, NULL, NULL, FALSE);

			_app_settrayicon (hwnd, install_type);

			break;
		}

		case RM_CONFIG_UPDATE:
		{
			_app_profile_load (hwnd, NULL);

			_app_changefilters (hwnd, TRUE, FALSE);

			break;
		}

		case RM_CONFIG_RESET:
		{
			_r_queuedlock_acquireexclusive (&lock_rules_config);
			_r_obj_clearhashtable (rules_config);
			_r_queuedlock_releaseexclusive (&lock_rules_config);

			_r_path_makebackup (profile_info.profile_path, TRUE);

			_app_profile_initialize ();
			_app_profile_load (hwnd, NULL);

			_app_changefilters (hwnd, TRUE, FALSE);

			break;
		}

		case RM_TRAYICON:
		{
			switch (LOWORD (lparam))
			{
				case NIN_KEYSELECT:
				{
					if (GetForegroundWindow () != hwnd)
						_r_wnd_toggle (hwnd, TRUE);

					break;
				}

				case WM_MBUTTONUP:
				{
					_r_ctrl_sendcommand (hwnd, IDM_TRAY_LOGSHOW, 0);
					break;
				}

				case WM_LBUTTONUP:
				{
					if (_r_config_getboolean (L"IsTrayIconSingleClick", TRUE))
					{
						_r_wnd_toggle (hwnd, FALSE);
					}
					else
					{
						SetForegroundWindow (hwnd);
					}

					break;
				}

				case WM_LBUTTONDBLCLK:
				{
					if (!_r_config_getboolean (L"IsTrayIconSingleClick", TRUE))
						_r_wnd_toggle (hwnd, FALSE);

					break;
				}

				case WM_CONTEXTMENU:
				{
					_app_message_traycontextmenu (hwnd);
					break;
				}
			}

			break;
		}

		case WM_NOTIFICATION:
		{
			PITEM_LOG ptr_log;
			HWND hnotify;

			if (_r_queuedlock_tryacquireexclusive (&lock_notify))
			{
				ptr_log = _r_obj_reference ((PITEM_LOG)lparam);
				hnotify = _app_notify_getwindow (ptr_log);

				_r_queuedlock_releaseexclusive (&lock_notify);

				SetWindowLongPtrW (hwnd, DWLP_MSGRESULT, (LONG_PTR)hnotify);

				return (INT_PTR)hnotify;
			}

			return 0;
		}

		case WM_CLOSE:
		{
			LPCWSTR cfg_name;
			UINT locale_id;

			if (_wfp_isfiltersinstalled ())
			{
				if (_app_istimersactive ())
				{
					cfg_name = L"ConfirmExitTimer";
					locale_id = IDS_QUESTION_TIMER;
				}
				else
				{
					cfg_name = L"ConfirmExit2";
					locale_id = IDS_QUESTION_EXIT;
				}

				if (!_r_show_confirmmessage (hwnd, _r_locale_getstring (locale_id), NULL, cfg_name, FALSE))
				{
					SetWindowLongPtrW (hwnd, DWLP_MSGRESULT, TRUE);

					return TRUE;
				}
			}

			DestroyWindow (hwnd);

			break;
		}

		case WM_DESTROY:
		{
			HANDLE hengine;

			_app_loginit (FALSE);

			_r_config_setlong (L"CurrentTab", _app_listview_getcurrent (hwnd));

			if (_r_queuedlock_islocked (&lock_apply))
			{
				_r_workqueue_waitforfinish (&wfp_queue);
				_r_workqueue_destroy (&wfp_queue);
			}

			_r_tray_destroy (hwnd, &GUID_TrayIcon);

			hengine = _wfp_getenginehandle ();

			_wfp_uninitialize (hengine, FALSE);

			_r_imagelist_destroy (config.himg_rules_small);
			_r_imagelist_destroy (config.himg_rules_large);
			_r_imagelist_destroy (config.himg_toolbar);

			BufferedPaintUnInit ();

			PostQuitMessage (0);

			break;
		}

		case WM_DROPFILES:
		{
			PR_STRING path;
			HDROP hdrop;
			ULONG_PTR app_hash = 0;
			ULONG numfiles;
			ULONG length;

			hdrop = (HDROP)wparam;
			numfiles = DragQueryFileW (hdrop, UINT32_MAX, NULL, 0);

			for (ULONG i = 0; i < numfiles; i++)
			{
				length = DragQueryFileW (hdrop, i, NULL, 0);
				path = _r_obj_createstring_ex (NULL, length * sizeof (WCHAR));

				if (DragQueryFileW (hdrop, i, path->buffer, length + 1))
				{
					_r_str_trimtonullterminator (&path->sr);

					app_hash = _app_addapplication (hwnd, DATA_UNKNOWN, path, NULL, NULL);
				}

				_r_obj_dereference (path);
			}

			DragFinish (hdrop);

			_app_profile_save (hwnd);

			if (!app_hash)
				break;

			_app_listview_updateby_param (hwnd, app_hash, PR_SETITEM_UPDATE, TRUE);
			_app_listview_showitemby_param (hwnd, app_hash, TRUE);

			break;
		}

		case WM_DPICHANGED:
		{
			_r_wnd_message_dpichanged (hwnd, wparam, lparam);

			_app_message_dpichanged (hwnd, LOWORD (wparam));

			break;
		}

		case WM_THEMECHANGED:
		{
			LONG dpi_value;

			dpi_value = _r_dc_getwindowdpi (hwnd);

			_app_windowloadfont (dpi_value);
			_app_listview_loadfont (dpi_value, TRUE);

			_app_listview_updateby_id (hwnd, DATA_LISTVIEW_CURRENT, PR_UPDATE_TYPE | PR_UPDATE_FORCE);

			_r_wnd_sendmessage (hwnd, 0, WM_SIZE, 0, 0);

			break;
		}

		case WM_SIZE:
		{
			RECT rect;
			LONG dpi_value;

			if (!GetClientRect (hwnd, &rect))
				break;

			dpi_value = _r_dc_getwindowdpi (hwnd);

			_app_window_resize (hwnd, &rect, dpi_value);

			break;
		}

		case WM_GETMINMAXINFO:
		{
			LPMINMAXINFO minmax;
			R_SIZE point = {0};
			LONG dpi_value;

			minmax = (LPMINMAXINFO)lparam;

			point.cx = 500;
			point.cy = 220;

			dpi_value = _r_dc_getwindowdpi (hwnd);

			_r_dc_getsizedpivalue (&point, dpi_value, TRUE);

			minmax->ptMinTrackSize.x = point.cx;
			minmax->ptMinTrackSize.y = point.cy;

			break;
		}

		case WM_NOTIFY:
		{
			LPNMHDR nmlp;

			nmlp = (LPNMHDR)lparam;

			switch (nmlp->code)
			{
				case TCN_SELCHANGING:
				{
					HWND hlistview;
					INT listview_id;

					listview_id = _app_listview_getcurrent (hwnd);

					if (!listview_id)
						break;

					hlistview = GetDlgItem (hwnd, listview_id);

					if (!hlistview)
						break;

					ShowWindow (hlistview, SW_HIDE);

					break;
				}

				case TCN_SELCHANGE:
				{
					HWND hlistview;
					INT listview_id;

					listview_id = _app_listview_getcurrent (hwnd);

					if (!listview_id)
						break;

					hlistview = GetDlgItem (hwnd, listview_id);

					if (!hlistview)
						break;

					_app_search_applyfilter (hwnd, listview_id, config.search_string);

					_app_listview_updateby_id (hwnd, listview_id, PR_UPDATE_FORCE | PR_UPDATE_NORESIZE);

					ShowWindow (hlistview, SW_SHOWNA);

					if (_r_wnd_isvisible (hwnd, TRUE)) // HACK!!!
						SetFocus (hlistview);

					_app_listview_resize (hwnd, listview_id, FALSE);

					break;
				}

				case NM_CUSTOMDRAW:
				{
					LONG_PTR result;

					result = _app_message_custdraw (hwnd, (LPNMLVCUSTOMDRAW)lparam);

					SetWindowLongPtrW (hwnd, DWLP_MSGRESULT, result);

					return result;
				}

				case LVN_INSERTITEM:
				{
					LPNMLISTVIEW lpnmlv;
					PITEM_LISTVIEW_CONTEXT context;
					INT listview_id;

					lpnmlv = (LPNMLISTVIEW)lparam;
					listview_id = (INT)(INT_PTR)lpnmlv->hdr.idFrom;

					if (!(listview_id >= IDC_APPS_PROFILE && listview_id <= IDC_LOG))
						break;

					if (_r_obj_isstringempty (config.search_string))
						break;

					context = (PITEM_LISTVIEW_CONTEXT)_r_listview_getitemlparam (hwnd, listview_id, lpnmlv->iItem);

					if (!context)
						break;

					_app_search_applyfilteritem (hwnd, listview_id, lpnmlv->iItem, context, config.search_string);

					break;
				}

				case LVN_DELETEITEM:
				{
					LPNMLISTVIEW lpnmlv;
					PITEM_LISTVIEW_CONTEXT context;
					INT listview_id;

					lpnmlv = (LPNMLISTVIEW)lparam;
					listview_id = (INT)(INT_PTR)lpnmlv->hdr.idFrom;

					if (!(listview_id >= IDC_APPS_PROFILE && listview_id <= IDC_LOG))
						break;

					context = (PITEM_LISTVIEW_CONTEXT)lpnmlv->lParam;

					if (!context)
						break;

					_app_listview_destroycontext (context);

					break;
				}

				case LVN_COLUMNCLICK:
				{
					LPNMLISTVIEW lpnmlv;
					INT ctrl_id;

					lpnmlv = (LPNMLISTVIEW)lparam;
					ctrl_id = (INT)(INT_PTR)lpnmlv->hdr.idFrom;

					_app_listview_sort (hwnd, ctrl_id, lpnmlv->iSubItem, TRUE);

					break;
				}

				case LVN_GETINFOTIP:
				{
					LPNMLVGETINFOTIPW lpnmlv;
					PR_STRING string;
					INT listview_id;

					lpnmlv = (LPNMLVGETINFOTIPW)lparam;
					listview_id = (INT)(INT_PTR)lpnmlv->hdr.idFrom;

					lparam = _app_listview_getitemcontext (hwnd, listview_id, lpnmlv->iItem);

					string = _app_gettooltipbylparam (hwnd, listview_id, lparam);

					if (!string)
						break;

					_r_str_copy (lpnmlv->pszText, lpnmlv->cchTextMax, string->buffer);

					_r_obj_dereference (string);

					break;
				}

				case LVN_ITEMCHANGING:
				{
					LPNMLISTVIEW lpnmlv;
					INT listview_id;
					ULONG_PTR app_hash;

					lpnmlv = (LPNMLISTVIEW)lparam;

					listview_id = (INT)(INT_PTR)lpnmlv->hdr.idFrom;

					if (_app_listview_islocked (hwnd, listview_id))
						break;

					if (!(lpnmlv->uChanged & LVIF_STATE))
						break;

					if (!lpnmlv->lParam)
						break;

					if ((listview_id >= IDC_APPS_PROFILE && listview_id <= IDC_APPS_UWP))
					{
						app_hash = _app_listview_getcontextcode (lpnmlv->lParam);

						if ((lpnmlv->uNewState & LVIS_STATEIMAGEMASK) == INDEXTOSTATEIMAGEMASK (1))
						{
							if (app_hash == config.my_hash)
							{
								if (!_r_show_confirmmessage (hwnd, L"WARNING!", SZ_WARNING_ME, NULL, FALSE))
								{
									SetWindowLongPtrW (hwnd, DWLP_MSGRESULT, TRUE);

									return TRUE;
								}
							}
						}
						else if ((lpnmlv->uNewState & LVIS_STATEIMAGEMASK) == INDEXTOSTATEIMAGEMASK (2))
						{
							if (app_hash == config.svchost_hash)
							{
								if (!_r_show_confirmmessage (hwnd, L"WARNING!", SZ_WARNING_SVCHOST, NULL, FALSE))
								{
									SetWindowLongPtrW (hwnd, DWLP_MSGRESULT, TRUE);

									return TRUE;
								}
							}
							else if (app_hash != config.my_hash)
							{
								if (!_r_show_confirmmessage (hwnd, NULL, _r_locale_getstring (IDS_QUESTION_ALLOW), L"ConfirmAllow", FALSE))
								{
									SetWindowLongPtrW (hwnd, DWLP_MSGRESULT, TRUE);

									return TRUE;
								}
							}
						}
					}

					break;
				}

				case LVN_ITEMCHANGED:
				{
					LPNMLISTVIEW lpnmlv;
					HANDLE hengine;
					PR_LIST rules;
					PITEM_APP ptr_app;
					PITEM_RULE ptr_rule;
					ULONG_PTR app_hash;
					ULONG_PTR rule_idx;
					INT listview_id;
					BOOLEAN is_changed = FALSE;
					BOOLEAN is_enabled;

					lpnmlv = (LPNMLISTVIEW)lparam;
					listview_id = (INT)(INT_PTR)lpnmlv->hdr.idFrom;

					if ((lpnmlv->uChanged & LVIF_STATE) == 0)
						break;

					if ((lpnmlv->uNewState & LVIS_STATEIMAGEMASK) == INDEXTOSTATEIMAGEMASK (1) || ((lpnmlv->uNewState & LVIS_STATEIMAGEMASK) == INDEXTOSTATEIMAGEMASK (2)))
					{
						if (_app_listview_islocked (hwnd, listview_id))
							break;

						is_enabled = (lpnmlv->uNewState & LVIS_STATEIMAGEMASK) == INDEXTOSTATEIMAGEMASK (2);

						if (listview_id >= IDC_APPS_PROFILE && listview_id <= IDC_APPS_UWP)
						{
							app_hash = _app_listview_getcontextcode (lpnmlv->lParam);
							ptr_app = _app_getappitem (app_hash);

							if (!ptr_app)
								break;

							if (ptr_app->is_enabled != is_enabled)
							{
								ptr_app->is_enabled = is_enabled;

								_app_listview_lock (hwnd, listview_id, TRUE);
								_app_setappiteminfo (hwnd, listview_id, lpnmlv->iItem, ptr_app);
								_app_listview_lock (hwnd, listview_id, FALSE);

								if (is_enabled)
									_app_notify_freeobject (NULL, ptr_app);

								if (!is_enabled && _app_istimerset (ptr_app))
									_app_timer_reset (hwnd, ptr_app);

								if (_wfp_isfiltersinstalled ())
								{
									hengine = _wfp_getenginehandle ();

									rules = _r_obj_createlist (1, NULL);

									_r_obj_addlistitem (rules, ptr_app);

									_wfp_create3filters (hengine, rules, DBG_ARG, FALSE);

									_r_obj_dereference (rules);
								}

								is_changed = TRUE;
							}

							_r_obj_dereference (ptr_app);
						}
						else if (listview_id >= IDC_RULES_BLOCKLIST && listview_id <= IDC_RULES_CUSTOM)
						{
							rule_idx = _app_listview_getcontextcode (lpnmlv->lParam);
							ptr_rule = _app_getrulebyid (rule_idx);

							if (!ptr_rule)
								break;

							if (ptr_rule->is_enabled != is_enabled)
							{
								_app_listview_lock (hwnd, listview_id, TRUE);

								_app_ruleenable (ptr_rule, is_enabled, TRUE);
								_app_setruleiteminfo (hwnd, listview_id, lpnmlv->iItem, ptr_rule, TRUE);

								_app_listview_lock (hwnd, listview_id, FALSE);

								if (_wfp_isfiltersinstalled ())
								{
									hengine = _wfp_getenginehandle ();

									rules = _r_obj_createlist (1, NULL);

									_r_obj_addlistitem (rules, ptr_rule);

									_wfp_create4filters (hengine, rules, DBG_ARG, FALSE);

									_r_obj_dereference (rules);
								}

								is_changed = TRUE;
							}

							_r_obj_dereference (ptr_rule);
						}

						if (is_changed)
						{
							_app_listview_updateby_id (hwnd, listview_id, 0);

							_app_profile_save (hwnd);
						}
					}

					break;
				}

				case LVN_GETDISPINFO:
				{
					LPNMLVDISPINFOW lpnmlv;
					INT listview_id;

					lpnmlv = (LPNMLVDISPINFOW)lparam;
					listview_id = (INT)(INT_PTR)lpnmlv->hdr.idFrom;

					_app_message_displayinfo (hwnd, listview_id, lpnmlv);

					break;
				}

				case LVN_GETEMPTYMARKUP:
				{
					NMLVEMPTYMARKUP* lpnmlv = (NMLVEMPTYMARKUP*)lparam;

					lpnmlv->dwFlags = EMF_CENTERED;

					_r_str_copy (lpnmlv->szMarkup, RTL_NUMBER_OF (lpnmlv->szMarkup), _r_locale_getstring (IDS_STATUS_EMPTY));

					SetWindowLongPtrW (hwnd, DWLP_MSGRESULT, TRUE);

					return TRUE;
				}

				case NM_DBLCLK:
				{
					LPNMITEMACTIVATE lpnmlv;
					LPNMMOUSE lpmouse;
					INT command_id = 0;
					INT listview_id;

					lpnmlv = (LPNMITEMACTIVATE)lparam;

					if (lpnmlv->iItem == INT_ERROR)
						break;

					listview_id = (INT)(INT_PTR)lpnmlv->hdr.idFrom;

					if (listview_id >= IDC_APPS_PROFILE && listview_id <= IDC_LOG)
					{
						command_id = IDM_PROPERTIES;
					}
					else if (listview_id == IDC_STATUSBAR)
					{
						lpmouse = (LPNMMOUSE)lparam;

						if (lpmouse->dwItemSpec == 0)
						{
							command_id = IDM_SELECT_ALL;
						}
						else if (lpmouse->dwItemSpec == 1)
						{
							command_id = IDM_PURGE_UNUSED;
						}
						else if (lpmouse->dwItemSpec == 2)
						{
							command_id = IDM_PURGE_TIMERS;
						}
					}

					if (command_id)
						_r_ctrl_sendcommand (hwnd, command_id, 0);

					break;
				}

				case NM_RCLICK:
				{
					LPNMITEMACTIVATE lpnmlv;

					if (nmlp->idFrom)
					{
						lpnmlv = (LPNMITEMACTIVATE)lparam;

						_app_message_contextmenu (hwnd, lpnmlv);
					}
					else
					{
						_app_message_contextmenu_columns (hwnd, nmlp);
					}

					break;
				}
			}

			break;
		}

		case WM_POWERBROADCAST:
		{
			if (_wfp_isfiltersapplying ())
				break;

			switch (wparam)
			{
				case PBT_APMSUSPEND:
				{
					HANDLE engine_handle;

					_app_logclear_ui (hwnd);

					if (config.is_neteventset)
					{
						engine_handle = _wfp_getenginehandle ();

						_wfp_logunsubscribe (engine_handle);
					}

					SetWindowLongPtrW (hwnd, DWLP_MSGRESULT, TRUE);

					return TRUE;
				}

				case PBT_APMRESUMECRITICAL:
				case PBT_APMRESUMESUSPEND:
				{
					HANDLE engine_handle;

					if (config.is_neteventset)
					{
						engine_handle = _wfp_getenginehandle ();

						_wfp_logsubscribe (hwnd, engine_handle);
					}

					SetWindowLongPtrW (hwnd, DWLP_MSGRESULT, TRUE);

					return TRUE;
				}
			}

			break;
		}

		case WM_DEVICECHANGE:
		{
			switch (wparam)
			{
				case DBT_DEVICEARRIVAL:
				{
					PDEV_BROADCAST_HDR lbhdr;
					PDEV_BROADCAST_VOLUME lpdbv;
					BOOLEAN is_appexist;

					if (_wfp_isfiltersapplying () || !_wfp_isfiltersinstalled ())
						break;

					lbhdr = (PDEV_BROADCAST_HDR)lparam;

					if (lbhdr && lbhdr->dbch_devicetype == DBT_DEVTYP_VOLUME)
					{
						lpdbv = (PDEV_BROADCAST_VOLUME)lparam;

						is_appexist = _app_isapphavedrive (FirstDriveFromMask (lpdbv->dbcv_unitmask));

						if (is_appexist)
							_app_changefilters (hwnd, TRUE, FALSE);
					}

					break;
				}
			}

			break;
		}

		case WM_COMMAND:
		{
			INT ctrl_id = LOWORD (wparam);
			INT notify_code = HIWORD (wparam);

			if (notify_code == EN_CHANGE)
			{
				PR_STRING string;
				INT listview_id;

				if (ctrl_id != IDC_SEARCH)
					break;

				listview_id = _app_listview_getcurrent (hwnd);
				string = _r_ctrl_getstring (config.hrebar, IDC_SEARCH);

				_r_obj_movereference (&config.search_string, string);

				_app_search_applyfilter (hwnd, listview_id, string);

				return FALSE;
			}
			else if (notify_code == 0)
			{
				if (ctrl_id >= IDX_LANGUAGE && ctrl_id <= IDX_LANGUAGE + (INT)(INT_PTR)_r_locale_getcount () + 1)
				{
					HMENU hmenu;
					HMENU hsubmenu;

					hmenu = GetMenu (hwnd);

					if (hmenu)
					{
						hsubmenu = GetSubMenu (GetSubMenu (hmenu, 2), LANG_MENU);

						if (hsubmenu)
							_r_locale_apply (hsubmenu, ctrl_id, IDX_LANGUAGE);
					}

					return FALSE;
				}
				else if (ctrl_id >= IDX_RULES_SPECIAL && ctrl_id <= (IDX_RULES_SPECIAL + (INT)(INT_PTR)_r_obj_getlistsize (rules_list)))
				{
					_app_command_idtorules (hwnd, ctrl_id);

					return FALSE;
				}
				else if (ctrl_id >= IDX_TIMER && ctrl_id <= (IDX_TIMER + (RTL_NUMBER_OF (timer_array) - 1)))
				{
					_app_command_idtotimers (hwnd, ctrl_id);

					return FALSE;
				}
			}

			switch (ctrl_id)
			{
				case IDCANCEL: // process Esc key
				{
					if (GetFocus () == config.hsearchbar)
					{
						_r_ctrl_setstring (config.hsearchbar, 0, L"");

						SetFocus (hwnd);

						break;
					}

					// fall through
				}

				case IDM_TRAY_SHOW:
				{
					_r_wnd_toggle (hwnd, FALSE);
					break;
				}

				case IDM_SETTINGS:
				case IDM_TRAY_SETTINGS:
				{
					_r_settings_createwindow (hwnd, &SettingsProc, 0);
					break;
				}

				case IDM_EXIT:
				case IDM_TRAY_EXIT:
				{
					_r_wnd_sendmessage (hwnd, 0, WM_CLOSE, 0, 0);
					break;
				}

				case IDM_WEBSITE:
				case IDM_TRAY_WEBSITE:
				{
					_r_shell_opendefault (_r_app_getwebsite_url ());
					break;
				}

				case IDM_CHECKUPDATES:
				{
					_r_update_check (hwnd);
					break;
				}

				case IDM_DONATE:
				{
					_r_shell_opendefault (_r_app_getdonate_url ());
					break;
				}

				case IDM_ABOUT:
				case IDM_TRAY_ABOUT:
				{
					_r_show_aboutmessage (hwnd);
					break;
				}

				case IDM_IMPORT:
				{
					static COMDLG_FILTERSPEC filters[] = {
						L"Profile files (*.xml)", L"*.xml",
						L"All files (*.*)", L"*.*",
					};

					R_FILE_DIALOG file_dialog;
					PR_STRING path;
					HRESULT status;

					status = _r_filedialog_initialize (&file_dialog, PR_FILEDIALOG_OPENFILE);

					if (SUCCEEDED (status))
					{
						_r_filedialog_setfilter (&file_dialog, filters, RTL_NUMBER_OF (filters));
						_r_filedialog_setpath (&file_dialog, XML_PROFILE_FILE);

						status = _r_filedialog_show (hwnd, &file_dialog);

						if (SUCCEEDED (status))
						{
							status = _r_filedialog_getpath (&file_dialog, &path);

							if (SUCCEEDED (status))
							{
								status = _app_profile_load (hwnd, path);

								if (NT_SUCCESS (status))
								{
									_app_profile_save (hwnd);

									_app_changefilters (hwnd, TRUE, FALSE);
								}

								_r_obj_dereference (path);
							}
						}

						_r_filedialog_destroy (&file_dialog);
					}

					break;
				}

				case IDM_EXPORT:
				{
					static COMDLG_FILTERSPEC filters[] = {
						L"Profile files (*.xml)", L"*.xml",
						L"All files (*.*)", L"*.*",
					};

					R_FILE_DIALOG file_dialog;
					PR_STRING path;
					NTSTATUS status;

					status = _r_filedialog_initialize (&file_dialog, PR_FILEDIALOG_SAVEFILE);

					if (SUCCEEDED (status))
					{
						_r_filedialog_setfilter (&file_dialog, filters, RTL_NUMBER_OF (filters));
						_r_filedialog_setpath (&file_dialog, XML_PROFILE_FILE);

						status = _r_filedialog_show (hwnd, &file_dialog);

						if (SUCCEEDED (status))
						{
							status = _r_filedialog_getpath (&file_dialog, &path);

							if (SUCCEEDED (status))
							{
								_app_profile_save (hwnd);

								// added information for export profile failure (issue #707)
								status = _r_fs_copyfile (&profile_info.profile_path->sr, &path->sr, FALSE);

								if (!NT_SUCCESS (status))
									_r_show_errormessage (hwnd, NULL, status, path->buffer, ET_NATIVE);

								_r_obj_dereference (path);
							}
						}

						_r_filedialog_destroy (&file_dialog);
					}

					break;
				}

				case IDM_ALWAYSONTOP_CHK:
				{
					BOOLEAN new_val;

					new_val = !_r_config_getboolean (L"AlwaysOnTop", FALSE);

					_r_menu_checkitem (GetMenu (hwnd), ctrl_id, 0, MF_BYCOMMAND, new_val);
					_r_config_setboolean (L"AlwaysOnTop", new_val);

					_r_wnd_top (hwnd, new_val);

					break;
				}

				case IDM_AUTOSIZECOLUMNS_CHK:
				{
					BOOLEAN new_val;

					new_val = !_r_config_getboolean (L"AutoSizeColumns", TRUE);

					_r_menu_checkitem (GetMenu (hwnd), ctrl_id, 0, MF_BYCOMMAND, new_val);
					_r_config_setboolean (L"AutoSizeColumns", new_val);

					if (new_val)
						_app_listview_resize (hwnd, _app_listview_getcurrent (hwnd), FALSE);

					break;
				}

				case IDM_SHOWFILENAMESONLY_CHK:
				{
					INT listview_id;
					BOOLEAN new_val;

					new_val = !_r_config_getboolean (L"ShowFilenames", TRUE);

					_r_menu_checkitem (GetMenu (hwnd), ctrl_id, 0, MF_BYCOMMAND, new_val);
					_r_config_setboolean (L"ShowFilenames", new_val);

					listview_id = _app_listview_getcurrent (hwnd);

					_r_listview_redraw (hwnd, listview_id);
					_app_listview_sort (hwnd, listview_id, INT_ERROR, FALSE);

					break;
				}

				case IDM_SHOWSEARCHBAR_CHK:
				{
					BOOLEAN new_val;

					new_val = !_r_config_getboolean (L"IsShowSearchBar", TRUE);

					_r_menu_checkitem (GetMenu (hwnd), ctrl_id, 0, MF_BYCOMMAND, new_val);

					_r_config_setboolean (L"IsShowSearchBar", new_val);

					if (config.hsearchbar)
						_app_search_setvisible (hwnd, config.hsearchbar, _r_dc_getwindowdpi (hwnd));

					break;
				}

				case IDM_VIEW_DETAILS:
				case IDM_VIEW_ICON:
				case IDM_VIEW_TILE:
				{
					LONG view_type;

					if (ctrl_id == IDM_VIEW_ICON)
					{
						view_type = LV_VIEW_ICON;
					}
					else if (ctrl_id == IDM_VIEW_TILE)
					{
						view_type = LV_VIEW_TILE;
					}
					else
					{
						view_type = LV_VIEW_DETAILS;
					}

					_r_menu_checkitem (GetMenu (hwnd), IDM_VIEW_DETAILS, IDM_VIEW_TILE, MF_BYCOMMAND, ctrl_id);

					_r_config_setlong (L"ViewType", view_type);

					_app_listview_updateby_id (hwnd, DATA_LISTVIEW_CURRENT, PR_UPDATE_TYPE);

					break;
				}

				case IDM_SIZE_SMALL:
				case IDM_SIZE_LARGE:
				case IDM_SIZE_EXTRALARGE:
				{
					LONG icon_size;

					if (ctrl_id == IDM_SIZE_LARGE)
					{
						icon_size = SHIL_LARGE;
					}
					else if (ctrl_id == IDM_SIZE_EXTRALARGE)
					{
						icon_size = SHIL_EXTRALARGE;
					}
					else
					{
						icon_size = SHIL_SMALL;
					}

					_r_menu_checkitem (GetMenu (hwnd), IDM_SIZE_SMALL, IDM_SIZE_EXTRALARGE, MF_BYCOMMAND, ctrl_id);

					_r_config_setlong (L"IconSize", icon_size);

					_app_listview_updateby_id (hwnd, DATA_LISTVIEW_CURRENT, PR_UPDATE_TYPE);

					break;
				}

				case IDM_ICONSISHIDDEN:
				{
					PITEM_APP ptr_app = NULL;
					ULONG_PTR enum_key = 0;
					INT listview_id;
					BOOLEAN new_val;

					new_val = !_r_config_getboolean (L"IsIconsHidden", FALSE);

					_r_menu_checkitem (GetMenu (hwnd), ctrl_id, 0, MF_BYCOMMAND, new_val);
					_r_config_setboolean (L"IsIconsHidden", new_val);

					if (!new_val)
					{
						_r_queuedlock_acquireshared (&lock_apps);

						while (_r_obj_enumhashtablepointer (apps_table, &ptr_app, NULL, &enum_key))
						{
							if (!ptr_app->real_path)
								continue;

							listview_id = _app_listview_getbytype (ptr_app->type);

							_app_getfileinformation (ptr_app->real_path, ptr_app->app_hash, ptr_app->type, listview_id);
						}

						_r_queuedlock_releaseshared (&lock_apps);
					}

					break;
				}

				case IDM_FONT:
				{
					_app_command_selectfont (hwnd);
					break;
				}

				case IDM_FIND:
				{
					if (!config.hsearchbar || !_r_wnd_isvisible (config.hsearchbar, FALSE))
						break;

					SetFocus (config.hsearchbar);

					_r_ctrl_setselection (config.hsearchbar, 0, MAKELPARAM (0, -1));

					break;
				}

				case IDM_REFRESH:
				{
					if (_wfp_isfiltersapplying ())
						break;

					_app_profile_load (hwnd, NULL);
					_app_changefilters (hwnd, TRUE, FALSE);

					break;
				}

				case IDM_LOADONSTARTUP_CHK:
				case IDM_STARTMINIMIZED_CHK:
				case IDM_SKIPUACWARNING_CHK:
				case IDM_CHECKUPDATES_CHK:
				case IDM_RULE_BLOCKOUTBOUND:
				case IDM_RULE_BLOCKINBOUND:
				case IDM_RULE_ALLOWLOOPBACK:
				case IDM_RULE_ALLOW6TO4:
				case IDM_RULE_ALLOWWINDOWSUPDATE:
				case IDM_PROFILETYPE_PLAIN:
				case IDM_PROFILETYPE_COMPRESSED:
				case IDM_PROFILETYPE_ENCRYPTED:
				case IDM_USENETWORKRESOLUTION_CHK:
				case IDM_USECERTIFICATES_CHK:
				case IDM_KEEPUNUSED_CHK:
				case IDM_USEHASHES_CHK:
				case IDM_USEAPPMONITOR_CHK:
				case IDM_USEDARKTHEME_CHK:
				{
					_app_config_apply (hwnd, NULL, ctrl_id);
					break;
				}

				case IDM_BLOCKLIST_SPY_DISABLE:
				case IDM_BLOCKLIST_SPY_ALLOW:
				case IDM_BLOCKLIST_SPY_BLOCK:
				case IDM_BLOCKLIST_UPDATE_DISABLE:
				case IDM_BLOCKLIST_UPDATE_ALLOW:
				case IDM_BLOCKLIST_UPDATE_BLOCK:
				case IDM_BLOCKLIST_EXTRA_DISABLE:
				case IDM_BLOCKLIST_EXTRA_ALLOW:
				case IDM_BLOCKLIST_EXTRA_BLOCK:
				{
					HMENU hmenu;
					LONG new_state;

					hmenu = GetMenu (hwnd);

					if (!hmenu)
						break;

					if (ctrl_id >= IDM_BLOCKLIST_SPY_DISABLE && ctrl_id <= IDM_BLOCKLIST_SPY_BLOCK)
					{
						_r_menu_checkitem (hmenu, IDM_BLOCKLIST_SPY_DISABLE, IDM_BLOCKLIST_SPY_BLOCK, MF_BYCOMMAND, ctrl_id);

						new_state = _r_calc_clamp (ctrl_id - IDM_BLOCKLIST_SPY_DISABLE, 0, 2);

						_r_config_setlong (L"BlocklistSpyState", new_state);

						_app_ruleblocklistset (hwnd, new_state, INT_ERROR, INT_ERROR, TRUE);
					}
					else if (ctrl_id >= IDM_BLOCKLIST_UPDATE_DISABLE && ctrl_id <= IDM_BLOCKLIST_UPDATE_BLOCK)
					{
						_r_menu_checkitem (hmenu, IDM_BLOCKLIST_UPDATE_DISABLE, IDM_BLOCKLIST_UPDATE_BLOCK, MF_BYCOMMAND, ctrl_id);

						new_state = _r_calc_clamp (ctrl_id - IDM_BLOCKLIST_UPDATE_DISABLE, 0, 2);

						_r_config_setlong (L"BlocklistUpdateState", new_state);

						_app_ruleblocklistset (hwnd, INT_ERROR, new_state, INT_ERROR, TRUE);
					}
					else if (ctrl_id >= IDM_BLOCKLIST_EXTRA_DISABLE && ctrl_id <= IDM_BLOCKLIST_EXTRA_BLOCK)
					{
						_r_menu_checkitem (hmenu, IDM_BLOCKLIST_EXTRA_DISABLE, IDM_BLOCKLIST_EXTRA_BLOCK, MF_BYCOMMAND, ctrl_id);

						new_state = _r_calc_clamp (ctrl_id - IDM_BLOCKLIST_EXTRA_DISABLE, 0, 2);

						_r_config_setlong (L"BlocklistExtraState", new_state);

						_app_ruleblocklistset (hwnd, INT_ERROR, INT_ERROR, new_state, TRUE);
					}

					break;
				}

				case IDM_TRAY_ENABLELOG_CHK:
				{
					BOOLEAN new_val;

					new_val = !_r_config_getboolean (L"IsLogEnabled", FALSE);

					_r_toolbar_setbutton (config.hrebar, IDC_TOOLBAR, ctrl_id, NULL, 0, new_val ? TBSTATE_PRESSED | TBSTATE_ENABLED : TBSTATE_ENABLED, I_IMAGENONE);

					_r_config_setboolean (L"IsLogEnabled", new_val);

					_app_loginit (new_val);

					break;
				}

				case IDM_TRAY_ENABLEUILOG_CHK:
				{
					BOOLEAN new_val;

					new_val = !_r_config_getboolean (L"IsLogUiEnabled", FALSE);

					_r_toolbar_setbutton (config.hrebar, IDC_TOOLBAR, ctrl_id, NULL, 0, new_val ? TBSTATE_PRESSED | TBSTATE_ENABLED : TBSTATE_ENABLED, I_IMAGENONE);

					_r_config_setboolean (L"IsLogUiEnabled", new_val);

					break;
				}

				case IDM_TRAY_ENABLENOTIFICATIONS_CHK:
				{
					HWND hnotify;
					BOOLEAN new_val;

					new_val = !_r_config_getboolean (L"IsNotificationsEnabled", TRUE);

					_r_toolbar_setbutton (config.hrebar, IDC_TOOLBAR, ctrl_id, NULL, 0, new_val ? TBSTATE_PRESSED | TBSTATE_ENABLED : TBSTATE_ENABLED, I_IMAGENONE);

					_r_config_setboolean (L"IsNotificationsEnabled", new_val);

					hnotify = _app_notify_getwindow (NULL);

					if (hnotify)
						_app_notify_refresh (hnotify);

					break;
				}

				case IDM_TRAY_ENABLENOTIFICATIONSSOUND_CHK:
				{
					BOOLEAN new_val;

					new_val = !_r_config_getboolean (L"IsNotificationsSound", TRUE);

					_r_config_setboolean (L"IsNotificationsSound", new_val);

					break;
				}

				case IDM_TRAY_NOTIFICATIONFULLSCREENSILENTMODE_CHK:
				{
					BOOLEAN new_val;

					new_val = !_r_config_getboolean (L"IsNotificationsFullscreenSilentMode", TRUE);

					_r_config_setboolean (L"IsNotificationsFullscreenSilentMode", new_val);

					break;
				}

				case IDM_TRAY_NOTIFICATIONONTRAY_CHK:
				{
					HWND hnotify;
					BOOLEAN new_val;

					new_val = !_r_config_getboolean (L"IsNotificationsOnTray", FALSE);

					_r_config_setboolean (L"IsNotificationsOnTray", new_val);

					hnotify = _app_notify_getwindow (NULL);

					if (hnotify)
						_app_notify_setposition (hnotify, TRUE);

					break;
				}

				case IDM_TRAY_LOGSHOW:
				{
					_app_command_logshow (hwnd);
					break;
				}

				case IDM_TRAY_LOGCLEAR:
				case IDM_LOGCLEAR:
				{
					_app_command_logclear (hwnd);
					break;
				}

				case IDM_TRAY_LOGSHOW_ERR:
				{
					_app_command_logerrshow (hwnd);
					break;
				}

				case IDM_TRAY_LOGCLEAR_ERR:
				{
					_app_command_logerrclear (hwnd);
					break;
				}

				case IDM_TRAY_START:
				{
					BOOLEAN is_filtersinstalled;

					if (_wfp_isfiltersapplying ())
						break;

					is_filtersinstalled = !_wfp_isfiltersinstalled ();

					if (_app_installmessage (hwnd, is_filtersinstalled))
						_app_changefilters (hwnd, is_filtersinstalled, TRUE);

					break;
				}

				case IDM_ADD_FILE:
				{
					static COMDLG_FILTERSPEC filters[] = {
						L"Executable files (*.exe)", L"*.exe",
						L"All files (*.*)", L"*.*",
					};

					R_FILE_DIALOG file_dialog;
					PR_STRING path;
					ULONG_PTR app_hash;
					HRESULT status;

					status = _r_filedialog_initialize (&file_dialog, PR_FILEDIALOG_OPENFILE);

					if (SUCCEEDED (status))
					{
						_r_filedialog_setfilter (&file_dialog, filters, RTL_NUMBER_OF (filters));

						status = _r_filedialog_show (hwnd, &file_dialog);

						if (SUCCEEDED (status))
						{
							status = _r_filedialog_getpath (&file_dialog, &path);

							if (SUCCEEDED (status))
							{
								app_hash = _app_addapplication (hwnd, DATA_UNKNOWN, path, NULL, NULL);

								if (app_hash)
								{
									_app_listview_updateby_param (hwnd, app_hash, PR_SETITEM_UPDATE, TRUE);
									_app_listview_showitemby_param (hwnd, app_hash, TRUE);

									_app_profile_save (hwnd);
								}

								_r_obj_dereference (path);
							}
						}

						_r_filedialog_destroy (&file_dialog);
					}

					break;
				}

				case IDM_DISABLENOTIFICATIONS:
				case IDM_DISABLEREMOVAL:
				case IDM_DISABLETIMER:
				{
					_app_command_disable (hwnd, ctrl_id);
					break;
				}

				case IDM_COPY:
				case IDM_COPY_VALUE:
				{
					_app_command_copy (hwnd, ctrl_id, (INT)lparam);
					break;
				}

				case IDM_EXPLORE:
				{
					PITEM_NETWORK ptr_network;
					PITEM_APP ptr_app;
					PITEM_LOG ptr_log;
					ULONG_PTR hash_code;
					INT listview_id;
					INT item_id = INT_ERROR;
					HRESULT status;

					listview_id = _app_listview_getcurrent (hwnd);

					if (listview_id >= IDC_APPS_PROFILE && listview_id <= IDC_APPS_UWP)
					{
						while ((item_id = _r_listview_getnextselected (hwnd, listview_id, item_id)) != INT_ERROR)
						{
							hash_code = _app_listview_getitemcontext (hwnd, listview_id, item_id);
							ptr_app = _app_getappitem (hash_code);

							if (ptr_app)
							{
								if (ptr_app->real_path)
								{
									if (_app_isappvalidpath (ptr_app->real_path))
									{
										status = _r_shell_showfile (&ptr_app->real_path->sr);

										if (FAILED (status))
											_r_show_errormessage (hwnd, L"Shell open file", status, ptr_app->real_path->buffer, ET_WINDOWS);
									}
								}

								_r_obj_dereference (ptr_app);
							}
						}
					}
					else if (listview_id == IDC_NETWORK)
					{
						while ((item_id = _r_listview_getnextselected (hwnd, listview_id, item_id)) != INT_ERROR)
						{
							hash_code = _app_listview_getitemcontext (hwnd, listview_id, item_id);
							ptr_network = _app_network_getitem (hash_code);

							if (ptr_network)
							{
								if (ptr_network->path)
								{
									if (_app_isappvalidpath (ptr_network->path))
									{
										status = _r_shell_showfile (&ptr_network->path->sr);

										if (FAILED (status))
											_r_show_errormessage (hwnd, L"Shell open file", status, ptr_network->path->buffer, ET_WINDOWS);
									}
								}

								_r_obj_dereference (ptr_network);
							}
						}
					}
					else if (listview_id == IDC_LOG)
					{
						while ((item_id = _r_listview_getnextselected (hwnd, listview_id, item_id)) != INT_ERROR)
						{
							hash_code = _app_listview_getitemcontext (hwnd, listview_id, item_id);
							ptr_log = _app_getlogitem (hash_code);

							if (ptr_log)
							{
								if (ptr_log->path)
								{
									if (_app_isappvalidpath (ptr_log->path))
									{
										status = _r_shell_showfile (&ptr_log->path->sr);

										if (FAILED (status))
											_r_show_errormessage (hwnd, L"Shell open file", status, ptr_log->path->buffer, ET_WINDOWS);
									}
								}

								_r_obj_dereference (ptr_log);
							}
						}
					}

					break;
				}

				case IDM_OPENRULESEDITOR:
				{
					_app_command_openeditor (hwnd);
					break;
				}

				case IDM_CHECK:
				case IDM_UNCHECK:
				{
					_app_command_checkbox (hwnd, ctrl_id);
					break;
				}

				case IDM_DELETE:
				{
					_app_command_delete (hwnd);
					break;
				}

				case IDM_PROPERTIES:
				{
					_app_command_properties (hwnd);
					break;
				}

				case IDM_PURGE_UNUSED:
				{
					_app_command_purgeunused (hwnd);
					break;
				}

				case IDM_PURGE_TIMERS:
				{
					_app_command_purgetimers (hwnd);
					break;
				}

				case IDM_SELECT_ALL:
				{
					INT listview_id;

					listview_id = _app_listview_getcurrent (hwnd);

					if (!listview_id)
						break;

					if (GetFocus () == GetDlgItem (hwnd, listview_id))
						_r_listview_setitemstate (hwnd, listview_id, INT_ERROR, LVIS_SELECTED, LVIS_SELECTED);

					break;
				}

				case IDM_ZOOM:
				{
					ShowWindow (hwnd, IsZoomed (hwnd) ? SW_RESTORE : SW_MAXIMIZE);
					break;
				}

				case IDM_TAB_NEXT:
				case IDM_TAB_PREV:
				{
					INT tabs_count;
					INT item_id;

					tabs_count = _r_tab_getitemcount (hwnd, IDC_TAB);

					if (!tabs_count)
						break;

					item_id = _r_tab_getcurrentitem (hwnd, IDC_TAB);

					if (item_id == INT_ERROR)
						break;

					if (ctrl_id == IDM_TAB_NEXT)
					{
						item_id += 1;

						if (item_id >= tabs_count)
							item_id = 0;
					}
					else
					{
						item_id -= 1;

						if (item_id == INT_ERROR)
							item_id = tabs_count - 1;
					}

					_r_tab_selectitem (hwnd, IDC_TAB, item_id);

					break;
				}
			}

			break;
		}
	}

	return FALSE;
}

BOOLEAN NTAPI _app_parseargs (
	_In_ R_CMDLINE_INFO_CLASS info_class
)
{
	HANDLE hengine;

	_app_initialize (NULL);

	hengine = _wfp_getenginehandle ();

	switch (info_class)
	{
		case CmdlineHelp:
		{
			_r_show_message (
				NULL,
				MB_OK | MB_ICONINFORMATION,
				L"Available options:",
				L"simplewall.exe -install - enable filtering.\r\n"
				L"simplewall.exe -install -temp - enable filtering until reboot.\r\n"
				L"simplewall.exe -install -silent - enable filtering without prompt.\r\n"
				L"simplewall.exe -uninstall - remove all installed filters.\r\n"
				L"simplewall.exe -help - show this message."
			);

			return TRUE;
		}

		case CmdlineInstall:
		{
			if (_r_sys_getopt (_r_sys_getcommandline (), L"silent", NULL) || _app_installmessage (NULL, TRUE))
			{
				if (_r_sys_getopt (_r_sys_getcommandline (), L"temp", NULL))
					config.is_filterstemporary = TRUE;

				_app_profile_initialize ();
				_app_profile_load (NULL, NULL);

				if (_wfp_initialize (NULL, hengine))
					_wfp_installfilters (hengine);

				_wfp_uninitialize (hengine, FALSE);
			}

			return TRUE;
		}

		case CmdlineUninstall:
		{
			if (_wfp_isfiltersinstalled () && _app_installmessage (NULL, FALSE))
			{
				_wfp_destroyfilters (hengine);
				_wfp_uninitialize (hengine, TRUE);
			}

			return TRUE;
		}
	}

	return FALSE;
}

INT APIENTRY wWinMain (
	_In_ HINSTANCE hinst,
	_In_opt_ HINSTANCE prev_hinst,
	_In_ LPWSTR cmdline,
	_In_ INT show_cmd
)
{
	HWND hwnd;
	ULONG result;

	if (!_r_app_initialize (&_app_parseargs))
		return ERROR_APP_INIT_FAILURE;

	hwnd = _r_app_createwindow (hinst, MAKEINTRESOURCE (IDD_MAIN), MAKEINTRESOURCE (IDI_MAIN), &DlgProc);

	if (!hwnd)
		return ERROR_APP_INIT_FAILURE;

	result = _r_wnd_message_callback (hwnd, MAKEINTRESOURCE (IDA_MAIN));

	return result;
}

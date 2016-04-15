#ifndef COPTIONSDIALOG_H
#define COPTIONSDIALOG_H

#include <memory>

#include "ui/wxInclude.h"

class wxNotebook;

namespace ui
{
class CGameConfigurations;
}

namespace hlmv
{
class CHLMVSettings;

class CGeneralOptions;

class COptionsDialog final : public wxDialog
{
public:
	COptionsDialog( wxWindow* pParent, CHLMVSettings* const pSettings );
	~COptionsDialog();

protected:
	wxDECLARE_EVENT_TABLE();

private:
	void OnButton( wxCommandEvent& event );

private:
	CHLMVSettings* const m_pSettings;
	std::unique_ptr<CHLMVSettings> m_EditableSettings;

	wxNotebook* m_pPages;

	CGeneralOptions* m_pGeneral;
	ui::CGameConfigurations* m_pGameConfigs;

private:
	COptionsDialog( const COptionsDialog& ) = delete;
	COptionsDialog& operator=( const COptionsDialog& ) = delete;
};
}

#endif //COPTIONSDIALOG_H
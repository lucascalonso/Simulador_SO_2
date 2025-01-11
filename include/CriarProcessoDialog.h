#ifndef CRIAR_PROCESSO_DIALOG_H
#define CRIAR_PROCESSO_DIALOG_H

#include <wx/wx.h>
#include "../include/Despachante.h"

class CriarProcessoDialog : public wxDialog {
public:
    CriarProcessoDialog(wxWindow* parent, Despachante* despachante);
private:
    wxTextCtrl *idCtrl, *duracaoCpu1Ctrl, *duracaoIoCtrl, *duracaoCpu2Ctrl, *ramCtrl;
    Despachante* despachanteInstance;
    void OnCriar(wxCommandEvent& event);
    void OnCancelar(wxCommandEvent& event);
    void OnFechar(wxCloseEvent& event);
};

#endif
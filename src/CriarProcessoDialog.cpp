#include "CriarProcessoDialog.h"

CriarProcessoDialog::CriarProcessoDialog(wxWindow* parent, Despachante* despachante)
    : wxDialog(parent, wxID_ANY, "Criar Processo", wxDefaultPosition, wxSize(400, 300)), despachanteInstance(despachante) {
    wxBoxSizer* sizer = new wxBoxSizer(wxVERTICAL);

    sizer->Add(new wxStaticText(this, wxID_ANY, "Id:"), 0, wxALL, 5);
    idCtrl = new wxTextCtrl(this, wxID_ANY);
    sizer->Add(idCtrl, 0, wxEXPAND | wxALL, 5);

    sizer->Add(new wxStaticText(this, wxID_ANY, "Duração CPU 1:"), 0, wxALL, 5);
    duracaoCpu1Ctrl = new wxTextCtrl(this, wxID_ANY);
    sizer->Add(duracaoCpu1Ctrl, 0, wxEXPAND | wxALL, 5);

    sizer->Add(new wxStaticText(this, wxID_ANY, "Duração IO:"), 0, wxALL, 5);
    duracaoIoCtrl = new wxTextCtrl(this, wxID_ANY);
    sizer->Add(duracaoIoCtrl, 0, wxEXPAND | wxALL, 5);

    sizer->Add(new wxStaticText(this, wxID_ANY, "Duração CPU 2:"), 0, wxALL, 5);
    duracaoCpu2Ctrl = new wxTextCtrl(this, wxID_ANY);
    sizer->Add(duracaoCpu2Ctrl, 0, wxEXPAND | wxALL, 5);

    sizer->Add(new wxStaticText(this, wxID_ANY, "RAM:"), 0, wxALL, 5);
    ramCtrl = new wxTextCtrl(this, wxID_ANY);
    sizer->Add(ramCtrl, 0, wxEXPAND | wxALL, 5);

    wxButton* criarButton = new wxButton(this, wxID_OK, "Criar");
    sizer->Add(criarButton, 0, wxALIGN_CENTER | wxALL, 10);

    SetSizerAndFit(sizer);

    Bind(wxEVT_BUTTON, &CriarProcessoDialog::OnCriar, this, wxID_OK);
    Bind(wxEVT_CLOSE_WINDOW, &CriarProcessoDialog::OnFechar, this);
}

void CriarProcessoDialog::OnCriar(wxCommandEvent& event) {
    // Obter os valores dos campos de entrada
    wxString IdStr = idCtrl->GetValue();
    wxString duracaoCpu1Str = duracaoCpu1Ctrl->GetValue();
    wxString duracaoIoStr = duracaoIoCtrl->GetValue();
    wxString duracaoCpu2Str = duracaoCpu2Ctrl->GetValue();
    wxString ramStr = ramCtrl->GetValue();

    // Verifique se os campos não estão vazios
    if (IdStr.IsEmpty() || duracaoCpu1Str.IsEmpty() || duracaoIoStr.IsEmpty() || duracaoCpu2Str.IsEmpty() || ramStr.IsEmpty()) {
        wxMessageBox("Por favor, preencha todos os campos antes de criar o processo.", "Erro", wxOK | wxICON_ERROR);
        return;
    }

    // Converter os valores para inteiros
    int id = wxAtoi(IdStr);
    int duracaoCpu1 = wxAtoi(duracaoCpu1Str);
    int duracaoIo = wxAtoi(duracaoIoStr);
    int duracaoCpu2 = wxAtoi(duracaoCpu2Str);
    int ram = wxAtoi(ramStr);

    // Criar o processo com os dados preenchidos
    Processo* novoProcesso = new Processo(id, duracaoCpu1, duracaoIo, duracaoCpu2, ram);
    despachanteInstance->tentaAlocarProcesso(novoProcesso);

    // Fechar o diálogo com sucesso
    EndModal(wxID_OK);
}

void CriarProcessoDialog::OnFechar(wxCloseEvent& event) {
    // Quando o usuário tenta fechar, apenas fecha o diálogo sem realizar nenhuma ação.
    EndModal(wxID_CANCEL);
}
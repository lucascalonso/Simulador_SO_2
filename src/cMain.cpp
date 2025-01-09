#include <wx/wx.h>
#include "../include/Processo.h"
#include "../include/GeradorDeProcessos.h"
#include "../include/Despachante.h"
#include "../include/GerenciadorMemoria.h"
#include "../include/globals.h"
#include <thread>
#include <chrono>


class MyFrame : public wxFrame {
public:
    MyFrame(GerenciadorMemoria* gm, GeradorDeProcessos* gp, Despachante* dp)
        : wxFrame(nullptr, wxID_ANY, "Simulador Round-robin", wxDefaultPosition, wxSize(1200, 900)),
          gerenciadorInstance(gm), geradorInstance(gp), despachanteInstance(dp) {
        
        wxButton* buttonEscalonar = new wxButton(this, wxID_ANY, "Escalonar", wxPoint(10, 500), wxSize(150, 40));
        wxButton* buttonEscalonarN = new wxButton(this, wxID_ANY, "Escalonar por N u.t", wxPoint(210, 500), wxSize(150, 40));
        wxButton* buttonGerarProcesso = new wxButton(this, wxID_ANY, "Gerar Processo", wxPoint(410, 500), wxSize(150, 40));
        wxButton* buttonImprimirMemoria = new wxButton(this, wxID_ANY, "Imprimir Memória", wxPoint(610, 500), wxSize(150, 40));
        
        wxFont font(12, wxFONTFAMILY_SWISS, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL, false, "Helvetica");
        buttonEscalonar->SetFont(font);
        buttonEscalonarN->SetFont(font);
        buttonGerarProcesso->SetFont(font);
        buttonImprimirMemoria->SetFont(font);
        
        buttonEscalonar->Bind(wxEVT_BUTTON, &MyFrame::OnButtonEscalonarClick, this);
        buttonEscalonarN->Bind(wxEVT_BUTTON, &MyFrame::OnButtonEscalonarNClick, this);
        buttonGerarProcesso->Bind(wxEVT_BUTTON, &MyFrame::OnButtonGerarProcessoClick, this);
        buttonImprimirMemoria->Bind(wxEVT_BUTTON, &MyFrame::OnButtonImprimirMemoriaClick, this);
        
        infoPanel = new wxPanel(this, wxID_ANY, wxPoint(0, 0), wxSize(900, 500));
        
        infoTextCtrl = new wxTextCtrl(infoPanel, wxID_ANY, "", wxPoint(10, 10), wxSize(880, 470),
                                      wxTE_MULTILINE | wxTE_READONLY | wxTE_WORDWRAP);
    }

private:
    GerenciadorMemoria* gerenciadorInstance;
    GeradorDeProcessos* geradorInstance;
    Despachante* despachanteInstance;
    
    wxPanel* infoPanel;
    wxTextCtrl* infoTextCtrl; 

    void OnButtonEscalonarClick(wxCommandEvent& event) {
        tempoAtual++;
        gerenciadorInstance->getDespachante()->escalonar();
        exibirTodosOsProcessos();
    }
    
    void OnButtonEscalonarNClick(wxCommandEvent& event) {
        wxMessageBox("Call método escalonar por n vezes", "Call escalonar N vezes", wxOK | wxICON_INFORMATION);
    }

    void OnButtonImprimirMemoriaClick(wxCommandEvent& event){
        gerenciadorInstance->visualizarMemoria();
        exibirTodosOsProcessos();
    }
    
    void OnButtonGerarProcessoClick(wxCommandEvent& event) {
        Processo* novoProcesso = geradorInstance->gerarProcesso();
        despachanteInstance->tentaAlocarProcesso(novoProcesso);
        
        exibirTodosOsProcessos();
    }

    void exibirTodosOsProcessos() {
        wxString infoTodosProcessos;

        // Iterando sobre o set de processos ordenados por ID
        for (auto& processo : despachanteInstance->getProcessosAtuais()) {
            wxString msg;
            msg.Printf(
                "ID: %d\nDuracao CPU1: %d\nDuracao IO: %d\nDuracao CPU2: %d\nRAM: %d MB\nTempo Chegada: %d\nEstado: %s\n\n", 
                processo->getId(), 
                processo->getDuracaoCpu1(),
                processo->getDuracaoIo(),
                processo->getDuracaoCpu2(),
                processo->getRam(),
                processo->getTempoChegada(),
                processo->getEstadoString()
            );
            infoTodosProcessos += msg;
        }
        updateInfoText(infoTodosProcessos);
    }

    void updateInfoText(const wxString& info) {
        //Atualiza o controle de texto com as informações passadas
        infoTextCtrl->SetValue(info); 

        //Garante que a área de texto seja redesenhada depois de atualizar
        infoTextCtrl->Refresh();
        infoTextCtrl->Update();
    }

    
};


class MyApp : public wxApp {
public:
    virtual bool OnInit() {
        Despachante* despachante = new Despachante(4,4);
        GerenciadorMemoria* gerenciador = new GerenciadorMemoria(32 * 1024, despachante, 4);
        despachante->setGerenciadorMemoria(gerenciador);
        GeradorDeProcessos* gerador = new GeradorDeProcessos(0.5);
        tempoAtual = 0;

        MyFrame* frame = new MyFrame(gerenciador,gerador,despachante);
        frame->Show(true);
        return true;
    }
};

wxIMPLEMENT_APP(MyApp);
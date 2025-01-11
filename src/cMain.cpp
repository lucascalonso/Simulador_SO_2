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
        : wxFrame(nullptr, wxID_ANY, "Simulador Round-robin", wxDefaultPosition, wxSize(1680, 900)),
          gerenciadorInstance(gm), geradorInstance(gp), despachanteInstance(dp), simuladorAtivo(false) {
        
        wxButton* buttonEscalonar = new wxButton(this, wxID_ANY, "Escalonar", wxPoint(10, 500), wxSize(150, 40));
        wxButton* buttonGerarProcesso = new wxButton(this, wxID_ANY, "Gerar Processo", wxPoint(210, 500), wxSize(150, 40));
        wxButton* buttonLigarSimulador = new wxButton(this, wxID_ANY, "Toggle Simulador", wxPoint(410, 500), wxSize(150, 40));
        wxButton* buttonImprimirMemoria = new wxButton(this, wxID_ANY, "Imprimir Memória", wxPoint(610, 500), wxSize(150, 40));
        
        wxFont font(10, wxFONTFAMILY_SWISS, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL, false, "Helvetica");
        buttonEscalonar->SetFont(font);
        buttonGerarProcesso->SetFont(font);
        buttonLigarSimulador->SetFont(font);
        buttonImprimirMemoria->SetFont(font);
        
        buttonEscalonar->Bind(wxEVT_BUTTON, &MyFrame::OnButtonEscalonarClick, this);
        buttonGerarProcesso->Bind(wxEVT_BUTTON, &MyFrame::OnButtonGerarProcessoClick, this);
        buttonLigarSimulador->Bind(wxEVT_BUTTON, &MyFrame::OnButtonLigarSimuladorClick, this);
        buttonImprimirMemoria->Bind(wxEVT_BUTTON, &MyFrame::OnButtonImprimirMemoriaClick, this);
        
        cpuPanel = new wxPanel(this, wxID_ANY, wxPoint(0, 0), wxSize(300, 500));
        processosPanel = new wxPanel(this, wxID_ANY, wxPoint(300, 0), wxSize(300, 500));

        memoriaPanel = new wxPanel(this, wxID_ANY, wxPoint(780, 10), wxSize(895, 448));  //wxSize(800,383)
        memoriaPanel->SetBackgroundColour(*wxWHITE);
        memoriaPanel->Bind(wxEVT_PAINT, &MyFrame::OnPaintMemoria, this);

        
        cpuTextCtrl = new wxTextCtrl(cpuPanel, wxID_ANY, "", wxPoint(10, 10), wxSize(880, 470),
                                      wxTE_MULTILINE | wxTE_READONLY | wxTE_WORDWRAP);

        processosTextCtrl = new wxTextCtrl(processosPanel, wxID_ANY, "", wxPoint(10, 10), wxSize(880, 470),
                                      wxTE_MULTILINE | wxTE_READONLY | wxTE_WORDWRAP);                              
    }

    ~MyFrame(){
        simuladorAtivo = false;
        if (simuladorThread.joinable()) {
            simuladorThread.join();
        }
    }

private:
    GerenciadorMemoria* gerenciadorInstance;
    GeradorDeProcessos* geradorInstance;
    Despachante* despachanteInstance;
    std::atomic<bool> simuladorAtivo;
    std::thread simuladorThread;
    
    wxPanel* memoriaPanel;
    wxPanel* cpuPanel;
    wxPanel* processosPanel;
    wxTextCtrl* cpuTextCtrl;
    wxTextCtrl* processosTextCtrl;

    //
    void OnButtonLigarSimuladorClick(wxCommandEvent& event) {
        if (!simuladorAtivo) {
            simuladorAtivo = true;
            simuladorThread = std::thread(&MyFrame::rodarSimulador, this);
            wxMessageBox("Simulador ligado!", "Simulador ligado!", wxOK | wxICON_INFORMATION);
        } else {
            simuladorAtivo = false;
            if (simuladorThread.joinable()) {
                simuladorThread.join();
            }
            wxMessageBox("Simulador desligado!", "Simulador desligado!", wxOK | wxICON_INFORMATION);
        }
    }

    //Método que 
    void rodarSimulador() {
        while (simuladorAtivo) {
            //Gera novos processos e tenta aloca-los
            std::vector<Processo*> novosProcessos = geradorInstance->gerarProcessos();
            for (Processo* processo : novosProcessos) {
                despachanteInstance->tentaAlocarProcesso(processo);
            }

            //Incrementa tempo e escalona processos
            tempoAtual++;
            gerenciadorInstance->getDespachante()->escalonar();

            //Atualiza a exibição dos processos (thread-safe chamada p/ a UI)
            CallAfter([this]() { exibirProcessosNasCpus(); });
            CallAfter([this]() { exibirTodosOsProcessos(); });

            //Desenha o painel de memória
            CallAfter([this]() { memoriaPanel->Refresh(); });

            //Pausa por 2 segundos
            std::this_thread::sleep_for(std::chrono::milliseconds(2000));
        }
    }

    void OnButtonEscalonarClick(wxCommandEvent& event) {
        tempoAtual++;
        gerenciadorInstance->getDespachante()->escalonar();
        exibirProcessosNasCpus();
        exibirTodosOsProcessos();
        memoriaPanel->Refresh();
    }
    

    void OnButtonImprimirMemoriaClick(wxCommandEvent& event) {
        memoriaPanel->Refresh();
        memoriaPanel->Update();
    }
    
    void OnButtonGerarProcessoClick(wxCommandEvent& event) {
        Processo* novoProcesso = geradorInstance->gerarProcesso();
        despachanteInstance->tentaAlocarProcesso(novoProcesso);
        
        exibirTodosOsProcessos();
        memoriaPanel->Refresh();
    }

    void exibirTodosOsProcessos() {
        
        //Limpa o texto existente antes de adicionar novos dados
        processosTextCtrl->Clear(); 

        //Iterando sobre o set de todos os processos ordenados por ID
        for (auto& processo : despachanteInstance->getProcessosAtuais()) {
            int processoId = processo->getId();
            
            //Gera a cor p/ o processo baseado no ID
            int r = (processoId * 50) % 256;  // R (vermelho) baseado no ID
            int g = (processoId * 75) % 256;  // G (verde) baseado no ID
            int b = (processoId * 100) % 256; // B (azul) baseado no ID
            wxColour cor(r, g, b);

            //Estilo com a cor e fonte personalizada
            wxTextAttr textoCor(cor);
            wxFont fonte(12, wxFONTFAMILY_SWISS, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_BOLD, false, "Helvetica");
            textoCor.SetFont(fonte);

            //Gera o texto do processo
            wxString msg;
            msg.Printf(
                "ID: %d\nDuracao Restante CPU: %d\nDuracao IO: %d\nRAM: %d MB\nTempo Chegada: %d\nEstado: %s\n\n", 
                processo->getId(), 
                processo->getTempoRestanteCpu(),
                processo->getDuracaoIo(),
                processo->getRam(),
                processo->getTempoChegada(),
                processo->getEstadoString()
            );

            //Determina o intervalo antes e depois de adicionar o texto
            long inicio = processosTextCtrl->GetLastPosition();
            processosTextCtrl->AppendText(msg);
            long fim = processosTextCtrl->GetLastPosition();

            //Aplica o estilo ao intervalo
            processosTextCtrl->SetStyle(inicio, fim, textoCor);
        }

        // Garante que o controle seja redesenhado
        processosTextCtrl->Refresh();
        processosTextCtrl->Update();
    }

    void exibirProcessosNasCpus() {
        cpuTextCtrl->Clear(); // Limpa o texto existente antes de adicionar novos dados.

        // Iterando sobre o set de processos ordenados pelo Id
        for (auto& processo : despachanteInstance->getProcessosAlocadosNoQuantum()) {    
                
            int processoId = processo->getId();
                
            // Gerar a cor para o processo baseado no ID
            int r = (processoId * 50) % 256;  // R (vermelho) baseado no ID
            int g = (processoId * 75) % 256;  // G (verde) baseado no ID
            int b = (processoId * 100) % 256; // B (azul) baseado no ID
            wxColour cor(r, g, b);

             // Criar um estilo com a cor e fonte personalizada
            wxTextAttr textoCor(cor);
            wxFont fonte(12, wxFONTFAMILY_SWISS, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_BOLD, false, "Helvetica");
            textoCor.SetFont(fonte);

            // Gerar o texto do processo
            wxString msg;
            msg.Printf(
                 "ID: %d\nDuracao Restante CPU: %d\nDuracao IO: %d\nEstado após executar: %s\n\n", 
                processo->getId(), 
                  processo->getTempoRestanteCpu(),
                  processo->getDuracaoIo(),
                 processo->getEstadoString()
            );

             // Determina o intervalo antes e depois de adicionar o texto
            long inicio = cpuTextCtrl->GetLastPosition();
            cpuTextCtrl->AppendText(msg);
            long fim = cpuTextCtrl->GetLastPosition();

            // Aplica o estilo ao intervalo recém-adicionado
            cpuTextCtrl->SetStyle(inicio, fim, textoCor);
        }

        // Garante que o controle seja redesenhado
        cpuTextCtrl->Refresh();
        cpuTextCtrl->Update();
    }
    


    void updateInfoText(const wxString& info) {
        //Atualiza o controle de texto com as informações passadas
        cpuTextCtrl->SetValue(info); 

        //Garante que a área de texto seja redesenhada depois de atualizar
        cpuTextCtrl->Refresh();
        cpuTextCtrl->Update();
    }

    void OnPaintMemoria(wxPaintEvent& event) {
        wxPaintDC dc(memoriaPanel); // Usando wxDC para desenhar no painel
        dc.SetBackground(*wxWHITE); // Definindo o fundo como branco
        dc.Clear(); // Limpando o painel antes de desenhar

        const int blockSize = 6;  // Tamanho de cada bloco (ajustado para ser um pouco maior)
        const int padding = 1;     // Espaço entre os blocos
        const int paginasPorLinha = 128; // Número de páginas por linha

        int x = 0, y = 0;

        // Desenha os blocos de memória
        for (int i = 0; i < gerenciadorInstance->getNumPaginas(); ++i) {
            // Obtém o ID do processo ou se é 0 (sem processo)
            int processoId = gerenciadorInstance->getMemoria()[i];

            wxColour cor;

            if (processoId == 0) {
                cor = wxColour(255, 255, 255);  // Cor branca para memória livre
            } else {
                // Gera cores baseadas no ID do processo
                int r = (processoId * 50) % 256;  // R (vermelho) baseado no ID
                int g = (processoId * 75) % 256;  // G (verde) baseado no ID
                int b = (processoId * 100) % 256; // B (azul) baseado no ID
                
                cor = wxColour(r, g, b);
                if (!cor.IsOk()) {
                    std::cerr << "Cor inválida gerada para o Processo #" << processoId << ": " << r << ", " << g << ", " << b << std::endl;
                    cor = wxColour(0, 0, 0); // Definir cor padrão caso seja inválida
                }
            }

            // Define a cor para o bloco
            dc.SetBrush(wxBrush(cor));
            dc.SetPen(wxPen(wxColor(0,0,0),1));

            // Desenha o bloco
            dc.DrawRectangle(x, y, blockSize, blockSize);

            // Atualiza a posição x e y
            x += blockSize + padding;
            if ((i + 1) % paginasPorLinha == 0) {
                x = 0;
                y += blockSize + padding; // Nova linha após atingir o número de páginas por linha
            }
        }
    }

};

class MyApp : public wxApp {
public:
    virtual bool OnInit() {
        
        Despachante* despachante = new Despachante(4,4);
        GerenciadorMemoria* gerenciador = new GerenciadorMemoria(32 * 1024, despachante, 4);
        despachante->setGerenciadorMemoria(gerenciador);
        GeradorDeProcessos* gerador = new GeradorDeProcessos(1);
        tempoAtual = 0;

        MyFrame* frame = new MyFrame(gerenciador,gerador,despachante);
        frame->Show(true);
        return true;
    }
};

wxIMPLEMENT_APP(MyApp);
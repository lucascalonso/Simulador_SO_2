#include <wx/wx.h>
#include <wx/utils.h>
#include "../include/Processo.h"
#include "../include/GeradorDeProcessos.h"
#include "../include/Despachante.h"
#include "../include/GerenciadorMemoria.h"
#include "../include/CriarProcessoDialog.h"
#include "../include/globals.h"
#include <random>
#include <thread>
#include <atomic>
#include <condition_variable>
#include <chrono>
#include <sstream>

//Casse para redirecionar couts para textoTextCtrl da Simulador
class wxCoutRedirector : public std::streambuf {
    public:
        wxCoutRedirector(wxWindow* parent, wxTextCtrl* textCtrl)
        : parent(parent),textCtrl(textCtrl), originalBuffer(std::cout.rdbuf()) {
        std::cout.rdbuf(this);
    }


        ~wxCoutRedirector() {
            std::cout.rdbuf(originalBuffer);
        }

        std::string getBuffer() {return buffer;}

    protected:
        virtual std::streamsize xsputn(const char* s, std::streamsize n) override {
            std::lock_guard<std::mutex> lock(bufferMutex);
            buffer.append(s, n);
            parent->CallAfter([this]() {
                flushToTextCtrl();
            });
            return n;
        }

        virtual int overflow(int c) override {
            if (c != EOF) {
                char buf = static_cast<char>(c);
                xsputn(&buf, 1);
            }
            return c;
        }

    private:
        wxTextCtrl* textCtrl;
        std::streambuf* originalBuffer;
        std::string buffer;
        std::mutex bufferMutex;
        wxWindow *parent;

        void flushToTextCtrl() {
            std::lock_guard<std::mutex> lock(bufferMutex);
            textCtrl->AppendText(buffer);
            buffer.clear();
            parent->CallAfter([this]() {
                textCtrl->Refresh();
            });
        }
};

//Frame principal da Simulador
class Simulador : public wxFrame {
public:
    Simulador(GerenciadorMemoria* gm, GeradorDeProcessos* gp, Despachante* dp)
        : wxFrame(nullptr, wxID_ANY, "Simulador Round-robin", wxDefaultPosition, wxSize(1680, 950)), 
          gerenciadorInstance(gm), geradorInstance(gp), despachanteInstance(dp), simuladorAtivo(false) {
        
        //definindo na ordem correta de inicialização para coutRedirector, buffer que display os couts dos métodos do backend
        textoPanel = new wxPanel(this, wxID_ANY, wxPoint(920, 520), wxSize(600, 385));
        textoTextCtrl = new wxTextCtrl(textoPanel, wxID_ANY, "", wxPoint(10, 10), wxSize(600, 360),
                                    wxTE_MULTILINE | wxTE_READONLY | wxTE_WORDWRAP);
        coutRedirector = new wxCoutRedirector(this, textoTextCtrl);
        
        mainSizer = new wxBoxSizer(wxVERTICAL);
        SetSizer(mainSizer);

        wxButton* buttonEscalonar = new wxButton(this, wxID_ANY, "Escalonar", wxPoint(10, 500), wxSize(150, 40));
        wxButton* buttonGerarProcesso = new wxButton(this, wxID_ANY, "Gerar Processo", wxPoint(210, 500), wxSize(150, 40));
        wxButton* buttonLigarSimulador = new wxButton(this, wxID_ANY, "Toggle Simulador", wxPoint(410, 500), wxSize(150, 40));
        wxButton* criarProcessoButton = new wxButton(this, wxID_ANY, "Criar Processo", wxPoint(610, 500), wxSize(150, 40));
        
        wxFont fontNegrito(12, wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_BOLD);
        textoTextCtrl->SetFont(fontNegrito);

        buttonEscalonar->SetFont(fontNegrito);
        buttonGerarProcesso->SetFont(fontNegrito);
        buttonLigarSimulador->SetFont(fontNegrito);
        criarProcessoButton->SetFont(fontNegrito);
        
        
        buttonEscalonar->Bind(wxEVT_BUTTON, &Simulador::OnButtonEscalonarClick, this);
        buttonGerarProcesso->Bind(wxEVT_BUTTON, &Simulador::OnButtonGerarProcessoClick, this);
        buttonLigarSimulador->Bind(wxEVT_BUTTON, &Simulador::OnButtonLigarSimuladorClick, this);
        criarProcessoButton->Bind(wxEVT_BUTTON,&Simulador::OnCriarProcesso, this);

        
        wxStaticText *textoCpu = new wxStaticText(this, wxID_ANY, "Processos executando", wxPoint(85, 5));
        textoCpu->SetFont(fontNegrito);

        wxStaticText *textoProcessos = new wxStaticText(this, wxID_ANY, "Processos em execução", wxPoint(390, 5));
        textoProcessos->SetFont(fontNegrito);

        wxStaticText *textoProntos = new wxStaticText(this, wxID_ANY, "Prontos", wxPoint(15, 555));
        textoProntos->SetFont(fontNegrito);

        wxStaticText *textoAuxiliar = new wxStaticText(this, wxID_ANY, "Auxiliar", wxPoint(15, 615));
        textoAuxiliar->SetFont(fontNegrito);

        wxStaticText *textoBloqueados = new wxStaticText(this, wxID_ANY, "Bloqueados", wxPoint(15, 675));
        textoBloqueados->SetFont(fontNegrito);

        wxStaticText *textoProntosSuspensos = new wxStaticText(this, wxID_ANY, "Prontos Suspensos", wxPoint(15, 735));
        textoProntosSuspensos->SetFont(fontNegrito);

        wxStaticText *textoBloqueadosSuspensos = new wxStaticText(this, wxID_ANY, "Bloqueados Suspensos", wxPoint(15, 795));
        textoBloqueadosSuspensos->SetFont(fontNegrito);

        wxStaticText *textoEventos = new wxStaticText(this, wxID_ANY, "Eventos", wxPoint(935, 515));
        textoEventos->SetFont(fontNegrito);


        memoriaDisponivelLabel = new wxStaticText(this, wxID_ANY, "Memória disponível: ", wxPoint(1150, 480));
        memoriaDisponivelLabel->SetFont(fontNegrito);

        tempoLabel = new wxStaticText(this, wxID_ANY, "Tempo Atual: ", wxPoint(15, 480));
        tempoLabel->SetFont(fontNegrito);

        cpuPanel = new wxPanel(this, wxID_ANY, wxPoint(0, 10), wxSize(300, 355));
        processosPanel = new wxPanel(this, wxID_ANY, wxPoint(300, 10), wxSize(300, 500));
        textoPanel = new wxPanel(this, wxID_ANY, wxPoint(780, 550), wxSize(900, 345));
        filaProntosPanel = new wxPanel(this, wxID_ANY, wxPoint(0, 560), wxSize(600, 40));
        filaAuxiliarPanel = new wxPanel(this, wxID_ANY, wxPoint(0, 620), wxSize(600, 40));
        filaBloqueadosPanel = new wxPanel(this, wxID_ANY, wxPoint(0, 680), wxSize(600, 40));
        filaProntosSuspensosPanel = new wxPanel(this, wxID_ANY, wxPoint(0, 740), wxSize(600, 40));
        filaBloqueadosSuspensosPanel = new wxPanel(this, wxID_ANY, wxPoint(0, 800), wxSize(600, 40));
        

        memoriaPanel = new wxPanel(this, wxID_ANY, wxPoint(780, 10), wxSize(895, 448),wxBORDER_NONE);
        memoriaPanel->SetBackgroundColour(*wxWHITE);
        memoriaPanel->Bind(wxEVT_PAINT, &Simulador::OnPaintMemoria, this);

        cpuPanel->Bind(wxEVT_PAINT, &Simulador::OnPaintPanel, this);
        textoPanel->Bind(wxEVT_PAINT, &Simulador::OnPaintPanel, this);
        processosPanel->Bind(wxEVT_PAINT, &Simulador::OnPaintPanel, this);
        filaProntosPanel->Bind(wxEVT_PAINT, &Simulador::OnPaintPanel, this);
        filaAuxiliarPanel->Bind(wxEVT_PAINT, &Simulador::OnPaintPanel, this);
        filaBloqueadosPanel->Bind(wxEVT_PAINT, &Simulador::OnPaintPanel, this);
        filaProntosSuspensosPanel->Bind(wxEVT_PAINT, &Simulador::OnPaintPanel, this);
        filaBloqueadosSuspensosPanel->Bind(wxEVT_PAINT, &Simulador::OnPaintPanel, this);

        filaProntosTextCtrl = new wxTextCtrl(filaProntosPanel, wxID_ANY, "", wxPoint(10, 10), wxSize(610, 40), 
                                 wxTE_MULTILINE | wxTE_READONLY | wxTE_WORDWRAP);
        filaProntosSuspensosTextCtrl = new wxTextCtrl(filaProntosSuspensosPanel, wxID_ANY, "", wxPoint(10, 10), wxSize(610, 40), 
                                 wxTE_MULTILINE | wxTE_READONLY | wxTE_WORDWRAP);
        filaBloqueadosTextCtrl = new wxTextCtrl(filaBloqueadosPanel, wxID_ANY, "", wxPoint(10, 10), wxSize(610, 40), 
                                 wxTE_MULTILINE | wxTE_READONLY | wxTE_WORDWRAP);
        filaBloqueadosSuspensosTextCtrl = new wxTextCtrl(filaBloqueadosSuspensosPanel, wxID_ANY, "", wxPoint(10, 10), wxSize(610, 40), 
                                 wxTE_MULTILINE | wxTE_READONLY | wxTE_WORDWRAP);
        filaAuxiliarTextCtrl = new wxTextCtrl(filaAuxiliarPanel, wxID_ANY, "", wxPoint(10, 10), wxSize(610, 40), 
                                 wxTE_MULTILINE | wxTE_READONLY | wxTE_WORDWRAP);                      
        
        cpuTextCtrl = new wxTextCtrl(cpuPanel, wxID_ANY, "", wxPoint(10, 10), wxSize(310, 365),
                                      wxTE_MULTILINE | wxTE_READONLY | wxTE_WORDWRAP);

        processosTextCtrl = new wxTextCtrl(processosPanel, wxID_ANY, "", wxPoint(10, 10), wxSize(880, 470),
                                      wxTE_MULTILINE | wxTE_READONLY | wxTE_WORDWRAP);                 

        SetSizer(mainSizer);
        Layout();
        atualizarMemoriaDisponivel();
        atualizarTempoAtual();                           
    }

    ~Simulador(){
        simuladorAtivo = false;
        if (geradorThread.joinable()) {
            geradorThread.join();
        }
        if (escalonadorThread.joinable()) {
            escalonadorThread.join();
        }

        delete coutRedirector;
    }

private:
    GerenciadorMemoria* gerenciadorInstance;
    GeradorDeProcessos* geradorInstance;
    Despachante* despachanteInstance;
    
    std::atomic<bool> simuladorAtivo;
    
    std::thread geradorThread;
    std::thread escalonadorThread;
    std::mutex mutex;
    std::condition_variable cv;
    
    
    wxPanel* memoriaPanel;
    wxPanel* cpuPanel;
    wxPanel* textoPanel;
    wxPanel* filaProntosPanel;
    wxPanel* filaProntosSuspensosPanel;
    wxPanel* filaBloqueadosPanel;
    wxPanel* filaBloqueadosSuspensosPanel;
    wxPanel* filaAuxiliarPanel;
    wxPanel* processosPanel;

    wxCoutRedirector *coutRedirector;

    wxStaticText *memoriaDisponivelLabel;
    wxStaticText *tempoLabel;
    wxStaticText *textoCpu;
    wxStaticText *textoProcessos;
    wxStaticText *textoProntos;
    wxStaticText *textoProntosSuspensos;
    wxStaticText *textoBloqueados;
    wxStaticText *textoBloqueadosSuspensos;
    wxStaticText *textoAuxiliar;
    wxStaticText *textoEventos;

    wxTextCtrl* cpuTextCtrl;
    wxTextCtrl* textoTextCtrl;
    wxTextCtrl* processosTextCtrl;
    wxTextCtrl* filaProntosTextCtrl;
    wxTextCtrl* filaProntosSuspensosTextCtrl;
    wxTextCtrl* filaBloqueadosTextCtrl;
    wxTextCtrl* filaBloqueadosSuspensosTextCtrl;
    wxTextCtrl* filaAuxiliarTextCtrl;

    wxBoxSizer* mainSizer;

    //Toggle do simulador após prompt e OK
    //As duas threads são pausadas por unidades de tempo diferente,
    //a thread geradora pausa por 1s e escalonador pausa por 2s
    //Cada thread também atualiza a parte pertinente da GUI
    void OnButtonLigarSimuladorClick(wxCommandEvent& event) {
        if (!simuladorAtivo) {
            simuladorAtivo = true;

            //Thread p/ geração de processos
            geradorThread = std::thread([this]() {
                while (simuladorAtivo) {
                    std::vector<Processo*> novosProcessos = geradorInstance->gerarProcessos();
                    for (Processo* processo : novosProcessos) {
                        despachanteInstance->tentaAlocarProcesso(processo);
                    }
                    //Atualiza os campos apropriados da GUI
                    CallAfter([this]() {
                        atualizarGUI();
                    });
                    //Pausa para não gerar tantos processos
                    std::this_thread::sleep_for(std::chrono::milliseconds(500)); 
                }
            });

            //Thread p/ escalonar
            escalonadorThread = std::thread([this]() {
                while (simuladorAtivo) {
                    {
                        std::unique_lock<std::mutex> lock(mutex);
                        cv.wait(lock, [this]() { return this->simuladorAtivo.load(); }); 
                    }

                    //Incrementa o tempo e chama escalonar
                    tempoAtual++;
                    gerenciadorInstance->getDespachante()->escalonar();

                    //Atualiza os campos da GUI
                    CallAfter([this]() {
                        atualizarGUI();
                    });
                    //Pausa p/ acompanhar
                    std::this_thread::sleep_for(std::chrono::milliseconds(2000)); 
                }
            });

            wxMessageBox("Simulador ligado!", "Simulador ligado!", wxOK | wxICON_INFORMATION);
        } else {
            simuladorAtivo = false;

            //Notifica as threads p/ unir
            cv.notify_all();

            if (geradorThread.joinable()) {
                geradorThread.join();
            }
            if (escalonadorThread.joinable()) {
                escalonadorThread.join();
            }

            wxMessageBox("Simulador desligado!", "Simulador desligado!", wxOK | wxICON_INFORMATION);
        }
    }

    //buffer para os couts no textoTextCtrl
    void flushToTextCtrl() {
        std::string buffer = coutRedirector->getBuffer();
        textoTextCtrl->AppendText(buffer);
        textoTextCtrl->ShowPosition(textoTextCtrl->GetLastPosition());
        buffer.clear();
    }

    void atualizarMemoriaDisponivel() {
        int memoriaDisponivel = gerenciadorInstance->getMemoriaDisponivel();
        int quadrosLivres = gerenciadorInstance->getQuadrosLivres();
        memoriaDisponivelLabel->SetLabel("Memória disponível: " + std::to_string(memoriaDisponivel) + " MB\nQuadros livres: "
                                                                + std::to_string(quadrosLivres));
    }

    void atualizarTempoAtual() {
        tempoLabel->SetLabel("Tempo Atual: " + std::to_string(tempoAtual));
    }


    void atualizarGUI(){
        atualizarTempoAtual();
        exibirProcessosNasCpus();
        exibirTodosOsProcessos();
        exibirTodasAsFilas();
        memoriaPanel->Refresh();
        atualizarMemoriaDisponivel();
    }

    void OnButtonEscalonarClick(wxCommandEvent& event) {
        tempoAtual++;
        atualizarTempoAtual();
        gerenciadorInstance->getDespachante()->escalonar();
        exibirProcessosNasCpus();
        exibirTodosOsProcessos();
        exibirTodasAsFilas();
        memoriaPanel->Refresh();
        textoPanel->Refresh();
        atualizarMemoriaDisponivel();
    }

    void OnPaintPanel(wxPaintEvent& event) {
        wxPaintDC dc(this);
        wxGraphicsContext* gc = wxGraphicsContext::Create(dc);

        if (gc) {
            wxPen pen(*wxBLACK, 1); 
            wxBrush brush(*wxWHITE);
            gc->SetPen(pen);
            gc->SetBrush(brush);
            gc->DrawRoundedRectangle(0, 0, GetSize().GetWidth(), GetSize().GetHeight(), 10);
        }

        event.Skip();
    }
    
    void OnButtonGerarProcessoClick(wxCommandEvent& event) {
        Processo* novoProcesso = geradorInstance->gerarProcesso();
        despachanteInstance->tentaAlocarProcesso(novoProcesso);
        exibirTodosOsProcessos();
        exibirTodasAsFilas();
        memoriaPanel->Refresh();
        textoPanel->Refresh();
        atualizarMemoriaDisponivel();
    }

    void exibirFila(const std::queue<Processo*>& fila,wxTextCtrl *textoCtrl){
        
        if(fila.empty()) return;

        std::queue<Processo*> copiaFila = fila;
        
        while (!copiaFila.empty()) {
            Processo *processo = copiaFila.front();
            copiaFila.pop();
        
            wxTextAttr textoCor(processo->getCor());
            wxFont fonte(28, wxFONTFAMILY_SWISS, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_BOLD, false, "Helvetica");
            textoCor.SetFont(fonte);

            //Gera o texto do processo
            wxString msg;
            msg.Printf(
                "%d    ", 
                processo->getId()
            );

            //Determina o intervalo antes e depois de adicionar o texto
            long inicio = textoCtrl->GetLastPosition();
            textoCtrl->AppendText(msg);
            long fim = textoCtrl->GetLastPosition();
            textoCtrl->SetStyle(inicio, fim, textoCor);
        }

        textoCtrl->Refresh();
        textoCtrl->Update();
        textoPanel->Refresh();
    }

    void exibirTodosOsProcessos() {
        
        //Limpa o texto existente antes de adicionar novos dados
        processosTextCtrl->Clear();
         

        //Iterando sobre o set de todos os processos ordenados por ID
        for (auto& processo : despachanteInstance->getProcessosAtuais()) {
            wxTextAttr textoCor(processo->getCor());
            wxFont fonte(12, wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_BOLD);
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

    void exibirTodasAsFilas(){
        limparTodasAsFilas();
        exibirFila(despachanteInstance->getFilaProntos(),filaProntosTextCtrl);
        exibirFila(despachanteInstance->getFilaProntosSuspensos(),filaProntosSuspensosTextCtrl);
        exibirFila(despachanteInstance->getFilaBloqueados(),filaBloqueadosTextCtrl);
        exibirFila(despachanteInstance->getFilaBloqueadosSuspensos(),filaBloqueadosSuspensosTextCtrl);
        exibirFila(despachanteInstance->getFilaAuxiliar(),filaAuxiliarTextCtrl);
    }

    void limparTodasAsFilas(){
        filaProntosTextCtrl->Clear();
        filaProntosSuspensosTextCtrl->Clear();
        filaBloqueadosSuspensosTextCtrl->Clear();
        filaBloqueadosTextCtrl->Clear();
        filaAuxiliarTextCtrl->Clear();
    }

    void exibirProcessosNasCpus() {
        //Limpa o texto existente
        cpuTextCtrl->Clear(); 
        
        //Para cada um das CPUs
        for (int i = 0; i < despachanteInstance->getNumCpus(); ++i) {
            
            auto& cpu = despachanteInstance->getCpusDisponiveis()[i];
            
            //Se o processo existe dentro da CPU, seleciona ele. Caso não exista, é porque terminou fase de I/O, e será igual a ultimoProcesso
            //Dessa forma, mesmo se P for selecionado para ser liberado, ainda teremos sua referência em ultimoProcesso até a próxima chamada de escalonar
            Processo* processo = cpu.P ? cpu.P : cpu.ultimoProcesso;

            wxString msg;
            
            //Check para podermos imprimir os processos marcados como finalizados e bloqueados nesse intervalo de tempo
            if (processo && (processo->getFezIo() || processo->getDuracaoIo() == processo->getDuracaoIoTotal())) {

                //Cria estilo com a cor e fonte personalizada
                wxTextAttr textoCor(processo->getCor());
                wxFont fonte(12, wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_BOLD);
                textoCor.SetFont(fonte);

                //Gera o texto p/ o frame
                msg.Printf(
                    "CPU #%d:\n"
                    "ID: %d\n"
                    "Count: %d\n"
                    "Duração Restante CPU: %d\n"
                    "Duração IO: %d\n"
                    "Estado: %s\n\n",
                    i + 1,
                    processo->getId(),
                    cpu.tempo_executando_processo,
                    processo->getTempoRestanteCpu(),
                    processo->getDuracaoIo(),
                    processo->getEstadoString()
                );

                //Determina o intervalo antes e depois de adicionar o texto
                long inicio = cpuTextCtrl->GetLastPosition();
                cpuTextCtrl->AppendText(msg);
                long fim = cpuTextCtrl->GetLastPosition();

                //Aplica o estilo ao intervalo recém-adicionado
                cpuTextCtrl->SetStyle(inicio, fim, textoCor);

            } else {
                //Nesse caso, a CPU está ociosa. Imprime assim como em escalonar()
                msg.Printf("CPU #%d: CPU Ocioso. Aguardando...\n\n", i + 1);
                cpuTextCtrl->AppendText(msg);
            }
        }
        cpuTextCtrl->Refresh();
        cpuTextCtrl->Update();
    }

    void OnPaintMemoria(wxPaintEvent& event) {
        wxPaintDC dc(memoriaPanel);
        dc.SetBackground(*wxWHITE);
        dc.Clear();

        const int blockSize = 6;
        const int padding = 1;
        const int paginasPorLinha = 128;

        int x = 0, y = 0;

        const auto& memoria = gerenciadorInstance->getMemoria();
        const int totalPaginas = gerenciadorInstance->getNumPaginas();
            
        for (int i = 0; i < totalPaginas;) {
            int processoId = memoria[i];
            int start = i;

            //Percorre blocos consecutivos com o mesmo processoId. Reduz overhead de chamadas de recuperarProcessosPorId
            while (i < totalPaginas && memoria[i] == processoId) {
                i++;
            }

            //Recupera cor do bloco. Se é 0 = memória livre e pinta de branco, caso contrário, pinta de processo->cor.
            wxColour cor;
            if (processoId == 0) {
                cor = wxColour(255, 255, 255);
            } else {
                Processo* processo = gerenciadorInstance->recuperarProcessoPorId(processoId);
                cor = processo->getCor();
            }
            for (int j = start; j < i; ++j) {
                dc.SetBrush(wxBrush(cor));
                dc.SetPen(wxPen(wxColor(0, 0, 0), 1));
                dc.DrawRectangle(x, y, blockSize, blockSize);
                x += blockSize + padding;
                if ((j + 1) % paginasPorLinha == 0) {
                    x = 0;
                    y += blockSize + padding;
                }
            }
        }
    }
    
    //Abre janela de criação de processo manual
    void OnCriarProcesso(wxCommandEvent& event) {
        CriarProcessoDialog dialog(this,despachanteInstance);  
        int resultado = dialog.ShowModal();

        if (resultado == wxID_OK) {
            memoriaPanel->Refresh();
            exibirTodosOsProcessos();
            exibirTodasAsFilas();
            atualizarMemoriaDisponivel();
        }
    };
};

class MyApp : public wxApp {
public:
    virtual bool OnInit() {
        
        Despachante* despachante = new Despachante(4,4);
        GerenciadorMemoria* gerenciador = new GerenciadorMemoria(32 * 1024, despachante, 4);
        despachante->setGerenciadorMemoria(gerenciador);
        GeradorDeProcessos* gerador = new GeradorDeProcessos(0.5);
        tempoAtual = 0;

        Simulador* frame = new Simulador(gerenciador,gerador,despachante);
        frame->Show(true);
        return true;
    }
};

wxIMPLEMENT_APP(MyApp);

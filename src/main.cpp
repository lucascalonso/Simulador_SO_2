#include "../include/Despachante.h"
#include "../include/GerenciadorMemoria.h"
#include <iostream>
#include "../include/globals.h"
#include "../include/GeradorDeProcessos.h"
#include <thread>
#include <chrono>


int main() {
    Despachante despachante(4, 4);
    GerenciadorMemoria gerenciador(32 * 1024, &despachante, 4);
    despachante.setGerenciadorMemoria(&gerenciador);

    //lambda = 0.5 (em média,  um processo novo a cada 2 u.t)
    GeradorDeProcessos gerador(0.5);

    while(true) {
        
        //Gera processos 
        std::vector<Processo*> novosProcessos = gerador.gerarProcessos();

        //Alocar cada novo processo no despachante
        for (auto processo : novosProcessos) {
            despachante.tentaAlocarProcesso(processo);
        }

        //Executar o escalonador e avançar o tempo
        despachante.escalonar();
        tempoAtual++;


        //Pausa de n milisegundos
        std::this_thread::sleep_for(std::chrono::milliseconds(2000));
    }

    std::cout << "Saindo...\n";
    return 0;
}

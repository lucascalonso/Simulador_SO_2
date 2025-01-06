#include "../include/Despachante.h"
#include "../include/GerenciadorMemoria.h"
#include <iostream>
#include "../include/globals.h"
#include "../include/GeradorDeProcessos.h"
#include <thread>
#include <chrono>
#include "../include/GeradorDeProcessos.h"

int main() {
    Despachante despachante(4, 4);
    GerenciadorMemoria gerenciador(32 * 1024, &despachante, 4);
    despachante.setGerenciadorMemoria(&gerenciador);

    GeradorDeProcessos gerador(1);

    int tempoParaProximo = gerador.tempoParaProximo();

    char continua;

    do {
        // Simula chegada de novos processos
        if (tempoAtual >= tempoParaProximo) {
            Processo* novoProcesso = gerador.gerarProximoProcesso();
            despachante.tentaAlocarProcesso(novoProcesso);

            std::cout << "Novo Processo Gerado - ID: " << novoProcesso->getId()
                      << ", RAM: " << novoProcesso->getRam() << " MB\n";

            tempoParaProximo = tempoAtual + gerador.tempoParaProximo();
        }
        despachante.escalonar();

        std::cout << "Tempo Atual: " << tempoAtual << "\n";
        std::cout << "Digite y ou Y para continuar.\n";
        std::cin >> continua;

        tempoAtual++;
    } while (continua == 'y' || continua == 'Y');

    std::cout << "Saindo...\n";
    return 0;
}

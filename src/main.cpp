#include "../include/Despachante.h"
#include "../include/GerenciadorMemoria.h"
#include <iostream>
#include "../include/globals.h"
#include <thread>
#include <chrono>

int main() {
    
    //Quantum 4 unidades de tempo e 4 Cpus
    Despachante despachante(4,4);
    
    //32GB                                   
    GerenciadorMemoria gerenciador (32*1024,&despachante,4);         
    despachante.setGerenciadorMemoria(&gerenciador);

    
    despachante.adicionarPronto(new Processo(1, 5, 8, 4, 12024));
    despachante.adicionarPronto(new Processo(2, 3, 10, 10, 8048));
    despachante.adicionarPronto(new Processo(3, 7, 0, 2, 1024));
    despachante.adicionarPronto(new Processo(4, 10, 15, 5, 2048));  
    despachante.adicionarPronto(new Processo(5, 4, 5, 3, 128));     
    despachante.adicionarPronto(new Processo(6, 15, 10, 10, 8192)); 
    despachante.adicionarPronto(new Processo(7, 12, 10, 8, 15000));  
    despachante.adicionarPronto(new Processo(8, 2, 3, 1, 5618)); 
    despachante.adicionarPronto(new Processo(9, 8, 4, 8, 10096));  
    
    char continua;

    //Simular botão da GUI para passo e outro p/ while com stop na thread 
    do{
        despachante.escalonar();
        std::cout << "Digite y ou Y para continuar.\n";
        std::cin >> continua;
        //std::this_thread::sleep_for(std::chrono::milliseconds(5000));


    } while (continua == 'y' || continua == 'Y');
    //1
    
    std::cout << "Saindo...\n";
    return 0;
}

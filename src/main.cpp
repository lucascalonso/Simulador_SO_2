#include "../include/Despachante.h"
#include "../include/GerenciadorMemoria.h"
#include <iostream>
#include <thread>
#include <wx/wxprec.h>

int main() {
    Despachante despachante(4,4);                                   //quantum 4 unidades de tempo e 4 Cpus
    GerenciadorMemoria gerenciador (32*1024,&despachante,4);         //mudar para 32 GB no final. 4 MB por página
    despachante.setGerenciadorMemoria(&gerenciador);

    
    despachante.adicionarPronto(new Processo(1, 5, 8, 4, 12024));
    despachante.adicionarPronto(new Processo(2, 3, 10, 0, 8048));
    despachante.adicionarPronto(new Processo(3, 7, 6, 2, 1024));
    despachante.adicionarPronto(new Processo(4, 10, 15, 5, 2048));  
    despachante.adicionarPronto(new Processo(5, 4, 5, 3, 128));     
    despachante.adicionarPronto(new Processo(6, 15, 10, 10, 8192)); 
    despachante.adicionarPronto(new Processo(7, 12, 10, 8, 15000));  
    despachante.adicionarPronto(new Processo(8, 2, 3, 1, 5618)); 
    despachante.adicionarPronto(new Processo(9, 8, 4, 8, 10096));  
    
    //delay 3s pra visualizar
    despachante.escalonar(3000);
    
    return 0;
}
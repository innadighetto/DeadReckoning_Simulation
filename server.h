//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU Lesser General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
// 
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Lesser General Public License for more details.
// 
// You should have received a copy of the GNU Lesser General Public License
// along with this program.  If not, see http://www.gnu.org/licenses/.
// 

#ifndef __DEADRECKONING_SERVER_H_
#define __DEADRECKONING_SERVER_H_

#include <omnetpp.h>
#include <string.h>
#include <chrono>
#include <random>
#include <stdlib.h>
#include <math.h>

using namespace omnetpp;
using namespace std;

/**
 * TODO - Generated class
 */
class Server : public cSimpleModule
{
    public:
        struct player{
              int ID;
              int x;
              int y;
              struct Server::player* next;
              struct Server::player* prev;
        };  // struttura che contiene le info di ogni player
        int nodi;   //nodi attualmente mantenuti nel sistema
        int max_node;   //massimo numero di players/nodi ammessi nel sistema
        int durata_ciclo_vita;  //durata di vita di un player prima della sua eliminazione dal gioco
        int matrix_lenght;  //larghezza matrice di gioco
        int *game_matrix;   //heap che mantiene le info sul feromone
        std::mt19937_64 random; //generatori di valori random con seme casuale
        struct player players_root; //mantiene il root della struttura usata per memorizzare tutti i player
        struct player *players_struct;  //usato per ciclare la struttura
        bool first_time = true; //valore che viene utilizzato per inizializzare delle operazioni
        int game_cycle; //discretizzazione del passo del gioco (azioni effettuale con cadenza di GAME_CYCLE secondi)
        int tolerance;  //tolleranza ammessa tra la posizione reale e quella del dead reckoning
        cRNG* random_fix;   //generatore di valori random con seme definito a priori
        int all_players;
        int removed_player;
        int rejected_player;
        int info_cycle;     //definisce ogni quanti istanti di simulazione stampare delle info riguardanti il sistema
        simsignal_t time_noPlayer;  //utilizzati per statistiche
        simsignal_t active_node;  //utilizzati per statistiche
        simsignal_t reject_node;  //utilizzati per statistiche
        simsignal_t statistic_positionFix; //utilizzati per statistiche
        simsignal_t statistic_positionFix_everyCycle;
        int num_positionFix;    //memorizza il numero totale di "aggiustamenti" fatti (quando il dead reckoning supera la tolleranza)
        int num_positionFix_Xcycle;
        int pheromone_unit;
        simsignal_t no_playerMoviment;  // numero volte che i giocatori non hanno fatto spostamenti preferendo la propria cella
        int no_moviment;    // numero volte che i giocatori non hanno fatto spostamenti preferendo la propria cella

    protected:
        bool gates[];

    protected:
        virtual void initialize();
        virtual void handleMessage(cMessage *msg);
        void getInfo(); //stampa a schermo delle informazioni sul sistema
        void deletePlayer(int id_to_delete);
        void insertPlayer();
        void move (int mossa, int* x, int* y);  //esegue le mosse definita dal dead reckoning
        int evaluateReckoningStep(int x, int y, int game_matrix_for_session[]); //valuta la mossa calcolando dead reckoning
};

#endif

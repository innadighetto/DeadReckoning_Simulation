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

#ifndef __DEADRECKONING_NODO_H_
#define __DEADRECKONING_NODO_H_

#include <omnetpp.h>
#include "server.h"
#include <chrono>
#include <random>
#include <string.h>
#include <stdlib.h>
#include <math.h>

using namespace omnetpp;
using namespace std;

/**
 * TODO - Generated class
 */
class Nodo : public cSimpleModule
{

    public:
        struct Server::player node_players_root;    //mantiene il root della struttura usata per memorizzare tutti i player
        struct Server::player *node_players_struct; //usato per ciclare la struttura
        int myID;
        int real_pos[2];  // 0 identifica la x, 1 la y
        int reckoning_pos[2];  // 0 identifica la x, 1 la y
        std::mt19937_64 random; //generatori di valori random con seme casuale
        int *game_matrix;   //heap che mantiene le info sul feromone
        int matrix_lenght;  //larghezza matrice di gioco
        int game_cycle; //discretizzazione del passo del gioco (azioni effettuale con cadenza di GAME_CYCLE secondi)
        int tolerance;  //tolleranza ammessa tra la posizione reale e quella del dead reckoning
        cRNG* random_fix;   //generatore di valori random con seme definito a priori
        int pheromone_unit;

    protected:
        virtual void initialize();
        virtual void handleMessage(cMessage *msg);
        void createThisNode(cMessage *msg);
        void deleteThisNode();
        void deletePlayer(int id_to_delete);
        void addPlayer(cMessage *msg);
        void move (int mossa, int* x, int* y, bool edit_pheromone); //esegue le mosse definita dal dead reckoning
        int evaluateReckoningStep(int x, int y, int game_matrix_for_session[]); //valuta la mossa calcolando dead reckoning
        void broadcast_real_pos();  //quando la tolleranza viene superata invia le info aggiornate al server
};

#endif

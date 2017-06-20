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

#include "nodo.h"
#include "node_creation_m.h"
#include "pos_update_m.h"

Define_Module(Nodo);

void Nodo::initialize()
{
    /*RANDOM viene creato con un seme casuale, non riproducibile*/
    unsigned seed = std::chrono::system_clock::now().time_since_epoch().count();
    random = std::mt19937_64(seed);
    /**/

    /*Imposto seme per il RANDOM_FIX che è un seme fissato e RIPRODUCIBILE*/
    random_fix = this->getRNG(0);

    game_cycle = (getSystemModule()->par("game_cycle").doubleValue());

    /*prima struttura standard e fissa*/
    node_players_root.prev = NULL;
    node_players_root.next = NULL;
    node_players_root.ID = NULL;
}

void Nodo::handleMessage(cMessage *msg)
{

    /*  MESSAGGI INVIATI DAL ME STESSO PER PROCEDERE CON LA GESTIONE DELLE POSIZIONI  */
    if(msg->isSelfMessage()){
        /*controllo che la posizione del dead reckoning non si discosti più di una certa tolleranza dalla posizione reale*/
        if (strcmp("check_pos", msg->getName()) == 0){
            /*controllo se valutare la distanza in modo diretto o tramite la matrice ciclica*/
            int middle_matrix = (int)(matrix_lenght/2);
            int edit_i = reckoning_pos[0];
            int edit_j = reckoning_pos[1];
            /*se la differenza tra le due celle in considerazione è maggiore della metà dell'ampiezza della matrice
             * di gioco allora vuol dire che posso arrivare da una cella all'altra percorrendo la matrice in modo
             * ciclico. Questo procedimento è fatto sia per le righe che per le colonne*/
            if (abs(reckoning_pos[0] - real_pos[0]) > middle_matrix){
                if(reckoning_pos[0] - real_pos[0] > 0){
                    edit_i = edit_i - matrix_lenght;
                }
                else if (reckoning_pos[0] - real_pos[0] < 0){
                    edit_i = edit_i + matrix_lenght;
                }
            }
            if (abs(reckoning_pos[1] - real_pos[1]) > middle_matrix){
                if(reckoning_pos[1] - real_pos[1] > 0){
                    edit_j = edit_j - matrix_lenght;
                }
                else if (reckoning_pos[1] - real_pos[1] < 0){
                    edit_j = edit_j + matrix_lenght;
                }
            }
            /*se supero la tolleranza informo il server*/
            if (abs(real_pos[0]-edit_i) >= tolerance || abs(real_pos[1]-edit_j) >= tolerance){
                /*invio al server la notifica che la posizione di dead reckoning è errata, bisogna sostituirla con la pos. reale*/
                broadcast_real_pos();

                // rimuovo il feromone dalla posizione del dead reckoning
                game_matrix[(reckoning_pos[0]*matrix_lenght)+reckoning_pos[1]] -= pheromone_unit;
                //aggiorno dead reckoning
                reckoning_pos[0] = real_pos[0];
                reckoning_pos[1] = real_pos[1];
                //aggiungo feromone nella posizione reale
                game_matrix[(real_pos[0]*matrix_lenght)+real_pos[1]] += pheromone_unit;
            }
            /*setta un messaggio da auto-inviarsi che servirà per valutare se il dead reckoning supera una certa tolleranza di errore*/
            cMessage *self_msg_check_pos = new cMessage("check_pos");
            scheduleAt(simTime()+game_cycle, self_msg_check_pos);
        }

        /*esegue il dead reckoning*/
        else if (strcmp("play", msg->getName()) == 0){
            /*CAMBIO LA POSIZIONE REALE IN MODO RANDOM*/
            int mossa = random_fix->intRand() % 9;
            move(mossa, &real_pos[0], &real_pos[1],0);
            mossa = -1;

            /*  Faccio una copia della matrice del feromone così da non modificarla con le nuove posizioni.
             *  La nuova matrice TEMPORANEA sarà usata per il dead reckoning
             *  in quanto deve essere eseguito su una versione solido e non modifica della matrice del feromone
             */
            int *tmp_game_matrix = (int*) malloc (sizeof(int*)*(matrix_lenght*matrix_lenght));
            for(int z=0; z<(matrix_lenght*matrix_lenght); z++){
                tmp_game_matrix[z] = game_matrix[z];
            }

            /*  importante: per ogni player i, prima di valutare il dead reckoning,
             *  devo togliere il suo valore di feromone sennò il giocatore i sarebbe "attratto" da se stesso
             */

            //DEAD RECKONING PLAYER NODO
            //rimuovo il feromone del player corrente per valutarne il deadReckoning
            int last_pos[2];
            last_pos[0] = reckoning_pos[0];
            last_pos[1] = reckoning_pos[1];
            tmp_game_matrix[(reckoning_pos[0]*matrix_lenght)+reckoning_pos[1]] -= pheromone_unit;
            //valuto la mossa di dead reckoning e la eseguo
            mossa = evaluateReckoningStep(reckoning_pos[0], reckoning_pos[1], tmp_game_matrix);
            move(mossa, &reckoning_pos[0], &reckoning_pos[1],1);
            //ri-aggiungo il feromone del player corrente che mi servirà per il corretto calcolo degli altri player
            tmp_game_matrix[(last_pos[0]*matrix_lenght)+last_pos[1]] += pheromone_unit;

            //DEAD RECKONING TUTTI GLI ALTRI PLAYERS
            node_players_struct = &node_players_root;
            while (node_players_struct->next != NULL){
                node_players_struct = node_players_struct->next;
                //rimuovo il feromone del player corrente per valutarne il deadReckoning
                last_pos[0] = node_players_struct->x;
                last_pos[1] = node_players_struct->y;
                tmp_game_matrix[(node_players_struct->x*matrix_lenght)+node_players_struct->y] -= pheromone_unit;
                //valuto la mossa di dead reckoning e la eseguo
                mossa = evaluateReckoningStep(node_players_struct->x,node_players_struct->y, tmp_game_matrix);
                move(mossa, &node_players_struct->x, &node_players_struct->y, 1);
                //ri-aggiungo il feromone del player corrente che mi servirà per il corretto calcolo degli altri player
                tmp_game_matrix[(last_pos[0]*matrix_lenght)+last_pos[1]] += pheromone_unit;
            }

            /*setta un messaggio da auto-inviarsi che servirà per svolgere le operazioni proprie del player che risiede in questo nodo*/
            cMessage *self_msg_move = new cMessage("play");
            scheduleAt(simTime()+game_cycle, self_msg_move);
        }
    }

    /*DELETE THIS NODE*/
    else if (strcmp("delete_node", msg->getName()) == 0){
        deleteThisNode();
    }

    /*ADD A PLAYER*/
    else if (strcmp("update", msg->getName()) == 0){
        addPlayer(msg);
    }

    /*DELETE A PLAYER*/
    else if (strcmp("deletePlayer", msg->getName()) == 0){
        int id_to_delete = msg->getKind();
        deletePlayer(id_to_delete);
    }

    /*CREATE THIS NODE*/
    else if (strcmp("creation", msg->getName()) == 0){
        createThisNode(msg);
        /*setta un messaggio da auto-inviarsi che servirà per valutare le mosse di questo player e di tutti gli altri.*/
        cMessage *self_msg_move = new cMessage("play");
        scheduleAt(simTime()+1, self_msg_move);

        /*setta un messaggio da auto-inviarsi che servirà per valutare se il dead reckoning supera una certa tolleranza di errore*/
        cMessage *self_msg_check_pos = new cMessage("check_pos");
        scheduleAt(simTime()+game_cycle, self_msg_check_pos);
    }

    /*UPDATE A PLAYER*/
    else if (strcmp("broadcast_update", msg->getName()) == 0){
        Pos_update *broadcast_update = (Pos_update*)msg;
        int id_sender = broadcast_update->getId();
        node_players_struct = &node_players_root;
        while (node_players_struct->next != NULL){
            node_players_struct = node_players_struct->next;
            if (node_players_struct->ID == id_sender){
                // rimuovo il feromone dalla vecchia posizione
                game_matrix[(node_players_struct->x*matrix_lenght)+node_players_struct->y] -= pheromone_unit;
                //aggiorno posizione
                node_players_struct->x = broadcast_update->getX();
                node_players_struct->y = broadcast_update->getY();
                //aggiungo feromone nella nuova posizione
                game_matrix[(node_players_struct->x*matrix_lenght)+node_players_struct->y] += pheromone_unit;
                break;
            }
        }
    }
}

void Nodo::deleteThisNode(){

    EV <<"\n Eliminato " << "Player " << myID <<" ---- " << "Posizione reale : [" <<real_pos[0] <<", " << real_pos[1] <<"]" <<"\n                    ---- Posizione deadReckoning : [" <<reckoning_pos[0] <<", " << reckoning_pos[1] <<"]" <<"\n";;

    EV <<"\n  Informazioni contenute \n";
    node_players_struct = &node_players_root;
    while (node_players_struct->next != NULL){
        node_players_struct = node_players_struct->next;
        EV << "Player " << node_players_struct->ID <<" ---- " << "Posizione : [" <<node_players_struct->x <<", " << node_players_struct->y <<"]" <<"\n";
    }
    for(int i=0; i<matrix_lenght; i++){
        EV <<"\n" ;
        for(int j=0; j<matrix_lenght; j++){
            EV <<game_matrix[(i*matrix_lenght)+j] <<"  ";
        }
    }
    EV <<"\n" ;

    /*elimino strutture dati del nodo*/
    free(game_matrix);
    node_players_struct = &node_players_root;
    while (node_players_struct->next != NULL){
        node_players_struct = node_players_struct->next;
    }
    while (node_players_struct->prev != NULL){
        node_players_struct = node_players_struct->prev;
        free(node_players_struct->next);
    }

    deleteModule();
}

void Nodo::createThisNode(cMessage *msg){
    Node_creation *creation_msg = (Node_creation*)msg;
    /*INIZIALIZZO MATRICE pheromone*/
    matrix_lenght = creation_msg->getMatrix_lenght();
    tolerance = creation_msg->getTolerance();
    pheromone_unit = creation_msg->getPheromone();
    game_matrix = (int*) malloc (sizeof(int*)*(matrix_lenght*matrix_lenght));
    for(int i=0; i<matrix_lenght*matrix_lenght; i++){
        game_matrix[i] = 0;
    }
    /*INIZIALIZZO STRUCT PLAYERS*/
    struct Server::player *tmp = &creation_msg->getPlayer_data();
    tmp = tmp->next;    //passo la prima struttura che è standard
    node_players_struct = &node_players_root;
    do{
        if (creation_msg->getID() == tmp->ID){
            real_pos[0] = tmp->x;
            reckoning_pos[0] = tmp->x;
            real_pos[1] = tmp->y;
            reckoning_pos[1] = tmp->y;
            myID = tmp->ID;
            game_matrix[(tmp->x*matrix_lenght)+tmp->y] += pheromone_unit;
        }
        else{
            node_players_struct->next = (Server::player*)malloc(sizeof(Server::player));
            node_players_struct->next->prev = node_players_struct;
            node_players_struct = node_players_struct->next;
            node_players_struct->ID = tmp->ID;
            node_players_struct->x = tmp->x;
            node_players_struct->y = tmp->y;
            node_players_struct->next = NULL;
            game_matrix[(tmp->x*matrix_lenght)+tmp->y] += pheromone_unit;
        }
        tmp = tmp->next;
    }while (tmp!=NULL);

    node_players_struct = &node_players_root;
    EV <<"\n    Creazione Player " <<myID << " ------ Posizione : [" <<reckoning_pos[0] <<", " <<reckoning_pos[1] <<"]" <<"\n" ;
    EV <<"Informazioni del nuovo player \n";
    while (node_players_struct->next != NULL){
        node_players_struct = node_players_struct->next;
        EV << "Player " << node_players_struct->ID <<" ---- " << "Posizione : [" <<node_players_struct->x <<", " << node_players_struct->y <<"]" <<"\n";
    }
    for(int i=0; i<matrix_lenght; i++){
        EV <<"\n" ;
        for(int j=0; j<matrix_lenght; j++){
            EV <<game_matrix[(i*matrix_lenght)+j] <<"  ";
        }
    }
    EV <<"\n" ;
}

void Nodo::addPlayer(cMessage *msg){
    bool add_player = true;

    Node_creation *update_msg = (Node_creation*)msg;
    int id_new_player = update_msg->getID();
    node_players_struct = &node_players_root;
    while (node_players_struct->next != NULL){
        node_players_struct = node_players_struct->next;
        /*se ho già questo ID non faccio nulla*/
        if (node_players_struct->ID == id_new_player){
            add_player = false;
        }
    }

    if (add_player){    //se non ho già le informazioni del giocatore in questione le aggiungo
        /*Ciclo per arrivare all'ultimo elemento della lista*/
        node_players_struct = &node_players_root;
        while (node_players_struct->next != NULL){
            node_players_struct = node_players_struct->next;
        }
        /*aggiungo elemento alla lista*/
        struct Server::player *tmp = &update_msg->getPlayer_data();
        node_players_struct->next = (Server::player*)malloc(sizeof(Server::player));
        node_players_struct->next->prev = node_players_struct;
        node_players_struct = node_players_struct->next;
        node_players_struct->ID = tmp->ID;
        node_players_struct->x = tmp->x;
        node_players_struct->y = tmp->y;
        node_players_struct->next = NULL;

        /*aggiorno matrice pheromone*/
        game_matrix[(node_players_struct->x*matrix_lenght)+node_players_struct->y] += pheromone_unit;
    }
}

void Nodo::deletePlayer(int id_to_delete){
    /*Elimino info player dalla struttura dati*/
    node_players_struct = &node_players_root;
    while(node_players_struct->next!=NULL){
        node_players_struct = node_players_struct->next;
        if (node_players_struct->ID == id_to_delete){
            /*Se il nodo da eliminare NON è l'ultimo della lista*/
            if (node_players_struct->next != NULL){
                node_players_struct->prev->next = node_players_struct->next;
                node_players_struct->next->prev = node_players_struct->prev;
            }
            /*Se il nodo da eliminare è l'ultimo della lista entro nell'ELSE*/
            else{
                node_players_struct->prev->next = NULL;
            }

            /*aggiorno matrice pheromone*/
            game_matrix[(node_players_struct->x*matrix_lenght)+node_players_struct->y] -= pheromone_unit;
            break;
        }
    }
}

/*  DEFINIZIONE DELLE 8 POSSIBILI MOSSE con X che rappresenta il player
 *
 *  0   1   2
 *  7   X   3
 *  6   5   4
 */
void Nodo::move(int mossa, int* x, int* y, bool edit_pheromone){
    if (edit_pheromone){
        game_matrix[((*x)*matrix_lenght)+(*y)] -= pheromone_unit;
    }
    switch(mossa){
        case 0: (*x)-1 == -1 ? *x = matrix_lenght-1 : *x -= 1;
                (*y)-1 == -1 ? *y = matrix_lenght-1 : *y -= 1;
                break;
        case 1: (*x)-1 == -1 ? *x = matrix_lenght-1 : *x -= 1;
                break;
        case 2: (*x)-1 == -1             ? *x = matrix_lenght-1 : *x -= 1;
                (*y)+1 == matrix_lenght  ? *y = 0               : *y += 1;
                break;
        case 3: (*y)+1 == matrix_lenght ? *y = 0 : *y += 1;
                break;
        case 4: (*x)+1 == matrix_lenght  ? *x = 0 : *x += 1;
                (*y)+1 == matrix_lenght  ? *y = 0 : *y += 1;
                break;
        case 5: (*x)+1 == matrix_lenght ? *x = 0 : *x += 1;
                break;
        case 6: (*x)+1 == matrix_lenght  ? *x = 0               : *x += 1;
                (*y)-1 == -1             ? *y = matrix_lenght-1 : *y -= 1;
                break;
        case 7: (*y)-1 == -1 ? *y = matrix_lenght-1 : *y -= 1;
                break;
        case 8: break; //rimane nella sua posizione
        default: break;
    }
    if (edit_pheromone){
        game_matrix[((*x)*matrix_lenght)+(*y)] += pheromone_unit;
    }
}

int Nodo::evaluateReckoningStep(int x, int y, int game_matrix_for_session[]){
    /* creo il vettore delle forze che attraggono il player in analisi*/
    float attr_forces_vect[2]; attr_forces_vect[0] = 0; attr_forces_vect[1] = 0;
    int num_attractive_cell = 0;

    int middle_matrix = (int)(matrix_lenght/2);

    for(int i=0; i<matrix_lenght; i++){
        for(int j=0; j<matrix_lenght; j++){
            int edit_i = i;
            int edit_j = j;
            //se nella cella corrente c'è almeno un player
            if(game_matrix_for_session[(edit_i*matrix_lenght)+edit_j] > 0){
                int feromone = game_matrix_for_session[(edit_i*matrix_lenght)+edit_j];
                /*controllo se valutare la distanza in modo diretto o tramite la matrice ciclica*/
                /*se la differenza tra le due celle in considerazione è maggiore della metà dell'ampiezza della matrice
                 * di gioco allora vuol dire che posso arrivare da una cella all'altra percorrendo la matrice in modo
                 * ciclico. Questo procedimento è fatto sia per le righe che per le colonne*/
                if (abs(edit_i - x) > middle_matrix){
                    if(edit_i - x > 0){
                        edit_i = edit_i - matrix_lenght;
                    }
                    else if (edit_i - x < 0){
                        edit_i = edit_i + matrix_lenght;
                    }
                }
                if (abs(edit_j - y) > middle_matrix){
                    if(edit_j - y > 0){
                        edit_j = edit_j - matrix_lenght;
                    }
                    else if (edit_j - y < 0){
                        edit_j = edit_j + matrix_lenght;
                    }
                }
                //creo vettore che rappresenta la distanza tra il player e la cella in analisi
                num_attractive_cell++;
                int vect[2];
                float distanza;
                float attractive_force;
                vect[0] = edit_i - x;
                vect[1] = edit_j - y;
                distanza = sqrt((vect[0]*vect[0])+(vect[1]*vect[1]));
                attractive_force = feromone/distanza;
                attr_forces_vect[0] += attractive_force*vect[0];
                attr_forces_vect[1] += attractive_force*vect[1];
            }
        }
    }
    /*divido i valori del vettore delle forze per il numero di celle che esercito la forza attrattiva e arrotondo per eccesso*/
    attr_forces_vect[0] = (int)((attr_forces_vect[0]/num_attractive_cell)+0.5);
    attr_forces_vect[1] = (int)((attr_forces_vect[1]/num_attractive_cell)+0.5);

    /*aggiungo alla posizione conosciuta il vettore delle forze risultante*/
    int desired_pos[2];
    desired_pos[0] = x + attr_forces_vect[0];
    desired_pos[1] = y + attr_forces_vect[1];


    /*determino la mossa da fare in quando, indipendentemente dalla posizione desiderata, il player può muoversi di una solo casella per volta*/
    int mossa = -1;
    if (desired_pos[0] > x){
        if (desired_pos[1] > y)
            mossa = 4;
        else if (desired_pos[1] == y)
            mossa = 5;
        else if (desired_pos[1] < y)
            mossa = 6;
    }
    else if (desired_pos[0] == x){
        if (desired_pos[1] > y)
            mossa = 3;
        else if (desired_pos[1] == y)
            mossa = -1;
        else if (desired_pos[1] < y)
            mossa = 7;
    }
    else if (desired_pos[0] < x){
        if (desired_pos[1] > y)
            mossa = 2;
        else if (desired_pos[1] == y)
            mossa = 1;
        else if (desired_pos[1] < y)
            mossa = 0;
    }

    return(mossa);
}

/*invio la nuova posizione(dopo aver verificato di superare la tolleranza) al server*/
void Nodo::broadcast_real_pos(){
    Pos_update *broadcast_update = new Pos_update("broadcast_update");
    broadcast_update->setId(myID);
    broadcast_update->setX(real_pos[0]);
    broadcast_update->setY(real_pos[1]);
    send(broadcast_update, "out", -1);
}

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

#include "server.h"
#include "node_creation_m.h"
#include "pos_update_m.h"

Define_Module(Server);

void Server::initialize()
{
    nodi=0;
    num_positionFix = 0;
    num_positionFix_Xcycle = 0;
    no_moviment = 0;

    /*statistic variable*/
    statistic_positionFix = registerSignal("positionFixSignal");
    emit(statistic_positionFix, 0);
    time_noPlayer = registerSignal("free");
    emit(time_noPlayer, true);
    active_node = registerSignal("activeNodeSignal");
    emit(active_node, 0);
    reject_node = registerSignal("rejectNodeSignal");
    emit(reject_node, 0);
    statistic_positionFix_everyCycle = registerSignal("positionFixSignalXcycle");
    emit(statistic_positionFix_everyCycle, 0);
    /**/
    no_playerMoviment = registerSignal("no_playerMoviment");
    emit(no_playerMoviment, 0);

    all_players=0;
    removed_player=0;
    rejected_player=0;
    info_cycle = par("info_cycle").doubleValue();
    max_node = par("node_number").doubleValue();
    durata_ciclo_vita = par("node_lifecycle").doubleValue();
    game_cycle = (getSystemModule()->par("game_cycle").doubleValue());
    pheromone_unit = par("pheromone_unit").doubleValue();
    tolerance = par("tolerance").doubleValue();

    /*RANDOM viene creato con un seme casuale, non riproducibile*/
    unsigned seed = std::chrono::system_clock::now().time_since_epoch().count();
    random = std::mt19937_64(seed);

    /*Imposto seme per il RANDOM_FIX che è un seme fissato e RIPRODUCIBILE*/
    random_fix = this->getRNG(0);

    /*inizializzazione matrice feromone*/
    matrix_lenght = par("game_matrix").doubleValue();
    game_matrix = (int*) malloc (sizeof(int*)*(matrix_lenght*matrix_lenght));
    for(int i=0; i<matrix_lenght*matrix_lenght; i++){
        game_matrix[i] = 0;
    }

    /*struttura ZERO standard e fissa dove aggiungerò tutti i players*/
    players_root.prev = NULL;
    players_root.next = NULL;
    players_root.ID = NULL;

    /* setto un messaggio che attiverà la stampa delle info del server */
       cMessage *info = new cMessage("info");
       scheduleAt(simTime()+info_cycle, info);
       /*setta un messaggio da auto-inviarsi che servirà per svolgere il dead reckoning su tutti i player*/
       cMessage *self_msg_move = new cMessage("play");
       scheduleAt(simTime()+game_cycle+1, self_msg_move);
       /*setta un messaggio da auto-inviarsi che servirà per aggiornare i dati statistici*/
       cMessage *self_msg_statistic = new cMessage("statistic");
       scheduleAt(simTime()+game_cycle+1, self_msg_statistic);
}

void Server::handleMessage(cMessage *msg)
{
    int gateIndex = -1;  // default gate index

    /*  MESSAGGI INVIATI DAL SERVER STESSO  */
    if(msg->isSelfMessage()){

        /*stampo informazioni contenute dal server*/
        if (strcmp("info", msg->getName()) == 0){
            getInfo();
            cMessage *info = new cMessage("info");
            scheduleAt(simTime()+info_cycle, info);
        }

        else if (strcmp("statistic", msg->getName()) == 0){
            //invio dati sui giocatori attivi
            emit(active_node, nodi);
            //invio dati sui giocatori rigettati
            emit(reject_node, rejected_player);
            rejected_player=0;
            //invio dati sugli aggiustamenti fatti nell'ultimo ciclo di gioco
            emit(statistic_positionFix_everyCycle, num_positionFix_Xcycle);
            num_positionFix_Xcycle=0;
            //invio dati sugli aggiustamenti fatti in totale
            emit(statistic_positionFix, num_positionFix);
            //invio dati sulle volte che i giocatori sono rimasti fermi, dead reckoning non ha trovato forze attrattive
            emit(no_playerMoviment, no_moviment);

            /*setta un messaggio da auto-inviarsi che servirà per aggiornare i dati statistici*/
            cMessage *self_msg_statistic = new cMessage("statistic");
            scheduleAt(simTime()+game_cycle, self_msg_statistic);
        }
        /*elimino le informazioni di un player e mando l'ordine di eliminare le sue info a tutto gli altri*/
        else if (strcmp("self_delete_node", msg->getName()) == 0){
            deletePlayer(msg->getKind());
        }

        /*esegue il dead reckoning*/
        else if (strcmp("play", msg->getName()) == 0){
            /*  Faccio una copia della matrice del feromone così da non modificarla con le nuove posizioni.
             *  La nuova matrice TEMPORANEA sarà usata per il dead reckoning
             *  in quanto deve essere eseguito su una versione solido e non modifica della matrice del feromone
             */
            int *tmp_game_matrix = (int*) malloc (sizeof(int*)*(matrix_lenght*matrix_lenght));
            for(int z=0; z<(matrix_lenght*matrix_lenght); z++){
                tmp_game_matrix[z] = game_matrix[z];
            }

            //CICLO TUTTI I PLAYER E FACCIO DEAD RECKONING
            /*  importante: per ogni player i, prima di valutare il dead reckoning,
             *  devo togliere il suo valore di feromone sennò il giocatore i sarebbe "attratto" da se stesso
             */
            int mossa = -1;
            int last_pos[2];
            players_struct = &players_root;
            while (players_struct->next != NULL){
                players_struct = players_struct->next;
                //rimuovo il feromone del player corrente dalla matrice temporanea
                last_pos[0] = players_struct->x;
                last_pos[1] = players_struct->y;
                tmp_game_matrix[(players_struct->x*matrix_lenght)+players_struct->y] -= pheromone_unit;
                //valuto la mossa di dead reckoning e la eseguo
                mossa = evaluateReckoningStep(players_struct->x,players_struct->y, tmp_game_matrix);
                move(mossa, &players_struct->x, &players_struct->y);
                //ri-aggiungo il feromone del player corrente che mi servirà per il corretto calcolo degli altri player
                tmp_game_matrix[(last_pos[0]*matrix_lenght)+last_pos[1]] += pheromone_unit;
            }

            /*setta un messaggio da auto-inviarsi che servirà per svolgere il dead reckoning su tutti i player*/
            cMessage *self_msg_move = new cMessage("play");
            scheduleAt(simTime()+game_cycle, self_msg_move);
        }
    }

    /*  MESSAGGI INVIATI DAI PLAYER CHE VOGLIO AGGIORNARE LA LORO POSIZIONE POICHE' ERRATA PIU' DI UNA CERTA TOLLERANZA*/
    else if (strcmp("broadcast_update", msg->getName()) == 0){
        num_positionFix++;
        num_positionFix_Xcycle++;
        Pos_update *tmp = (Pos_update*)msg;
        int id_sender = tmp->getId();
        int x_pos = tmp->getX();
        int y_pos = tmp->getY();

        players_struct = &players_root;
        while (players_struct->next != NULL){
            players_struct = players_struct->next;
            if (players_struct->ID != id_sender){   //id_sender mi identifica il nodo che ha mandato la richiesta
                int temp_gate = players_struct->ID;
                Pos_update *broadcast_update = new Pos_update("broadcast_update");
                broadcast_update->setId(id_sender);
                broadcast_update->setX((x_pos));
                broadcast_update->setY((y_pos));
                send(broadcast_update, "out", temp_gate);
            }
            else{   // se sto ciclando il player che ha mandato la richiesta, semplicemente aggiorno le sue info
                game_matrix[(players_struct->x*matrix_lenght)+players_struct->y] -= pheromone_unit;
                players_struct->x = x_pos;
                players_struct->y = y_pos;
                game_matrix[(players_struct->x*matrix_lenght)+players_struct->y] += pheromone_unit;
            }
        }
    }
    /*  MESSAGGI INVIATI DALLA SORGENTE OVVERO GESTISCO I NUOVI PLAYERS */
    else{
        if (nodi < max_node){
            all_players++;
            /* Creo dinamicamente i nuovi nodi e i rispettivi collegamenti con il nodo Server*/
            cGate *Server_OUT_gate;
            Server_OUT_gate = this->getOrCreateFirstUnconnectedGate("out",0,0,1);
            cModuleType *moduleType = cModuleType::get("deadreckoning.Nodo");

            /* Definisco il nome del nodo da creare */
            char gate_id[10];
            sprintf (gate_id, "Node %d", Server_OUT_gate->getIndex());
            /*itoa(Server_OUT_gate->getIndex(), gate_id, 10);
            char node_text[10] = "Node ";*/
            /**/

            //cModule *mod = moduleType->createScheduleInit(strcat(node_text,gate_id), this->getParentModule());
            cModule *mod = moduleType->create(gate_id, this->getParentModule());
            mod->finalizeParameters();

            /* collegamento dal nuovo nodo al Server */
            mod->gate("out")->connectTo(this->getOrCreateFirstUnconnectedGate("in",0,0,1));
            /* collegamento dal Server al nuovo nodo */
            Server_OUT_gate->connectTo(mod->gate("in"));

            mod->buildInside();
            mod->callInitialize();
            /*Fine creatione nuovo nodo*/

            // assegno il gate relativo al nodo appena creato
            gateIndex = Server_OUT_gate->getIndex();

            // controllo standard per controllare che il gate esista
            if (gateIndex < 0 || gateIndex >= gateSize("out"))
                throw cRuntimeError("Invalid output gate selected during routing");

            /*  Definisco e aggiungo le info del nuovo player alla struttura dati del server    */
            players_struct = &players_root;
            while (players_struct->next != NULL){
                players_struct = players_struct->next;
            }

            players_struct->next = (Server::player*)malloc(sizeof(Server::player));
            players_struct->next->prev = players_struct;
            players_struct = players_struct->next;

            players_struct->ID = gateIndex;
            players_struct->x = random_fix->intRand()%matrix_lenght;
            players_struct->y = random_fix->intRand()%matrix_lenght;
            /*players_struct->x = random()%matrix_lenght;
            players_struct->y = random()%matrix_lenght;*/
            game_matrix[(players_struct->x*matrix_lenght)+players_struct->y] += pheromone_unit;
            players_struct->next = NULL;

            /*setto una struttura temp che userò per inviare le info di questo nuovo player agli altri nodi*/
            struct player tmp_player_struct = *players_struct;
            tmp_player_struct.prev = NULL;
            /**/

            /* Send messaggio "CREAZIONE" al nodo appena creato */
            Node_creation *creation = new Node_creation("creation");
            creation->setPlayer_data(players_root);
            creation->setID(gateIndex);
            creation->setPheromone(pheromone_unit);
            creation->setMatrix_lenght(matrix_lenght);
            creation->setTolerance(tolerance);
            send(creation, "out", gateIndex);

            /* Send messaggio "ELIMINA" al nodo appena creato dopo X unità di tempo definito dalla variabile "durata_ciclo_vita" */
            cMessage *msg_del = new cMessage("delete_node");
            sendDelayed(msg_del, durata_ciclo_vita, "out", gateIndex);

            /* Send messaggio "ELIMINA" a se stesso(SERVER) cosi da eliminare le info del player appena creato dopo X unità di tempo */
            cMessage *self_msg_del = new cMessage("self_delete_node");
            self_msg_del->setKind(gateIndex);
            scheduleAt(simTime()+durata_ciclo_vita, self_msg_del);

            /* invio le informazioni del nuovo player a tutti quelli già esistenti*/
            players_struct = &players_root;
            while (players_struct->next != NULL){
                players_struct = players_struct->next;
                if (players_struct->ID != gateIndex){   //gateIndex mi identifica il nodo appena aggiunto a cui non devo inviare aggiornamenti
                    int temp_gate = players_struct->ID;

                    Node_creation *update = new Node_creation("update");
                    update->setPlayer_data(tmp_player_struct);
                    update->setID(tmp_player_struct.ID);
                    send(update, "out", temp_gate);
                }
            }
            nodi++;
            //invio valori per statistiche
            emit(time_noPlayer, false);
        }
        else if (nodi >= max_node){
            rejected_player++;
        }
    }
    /* inizializzo alcune funzioni se FIRST_TIME è true*/
    if (first_time){
        first_time = false;
    }
}

void Server::getInfo(){
    EV <<"\n\n Informazioni del SERVER : \n";

    players_struct = &players_root;
    while (players_struct->next != NULL){
        players_struct = players_struct->next;
        EV << "Player " << players_struct->ID <<" ---- " << "Posizione : [" <<players_struct->x <<", " << players_struct->y <<"]" <<"\n";
    }
    for(int i=0; i<matrix_lenght; i++){
        EV <<"\n" ;
        for(int j=0; j<matrix_lenght; j++){
            EV <<game_matrix[(i*matrix_lenght)+j] <<"  ";
        }
    }
    EV <<"\n" ;
    EV <<"Numero dei players eliminati ed attivi : " <<all_players <<"\n";
    EV <<"Numero dei players eliminati : " <<removed_player <<"\n";
    //EV <<"Numero dei players respinti : " <<rejected_player <<"\n";
    EV <<"Numero volte in cui il dead reckoning e' stato aggiustato : " <<num_positionFix <<"      -----------------------------------------------\n\n";
}

void Server::deletePlayer(int id_to_delete){
    nodi--;
    if (nodi == 0){
        //invio valori per statistiche
        emit(time_noPlayer, true);
    }
    removed_player++;
    /*Elimino info player dalla struttura dati*/
    struct Server::player *temp;
    temp = &players_root;
    while(temp->next!=NULL){
        temp = temp->next;
        if (temp->ID == id_to_delete){
            /*Se il nodo da eliminare NON è l'ultimo della lista*/
            if (temp->next != NULL){
                temp->prev->next = temp->next;
                temp->next->prev = temp->prev;
            }
            /*Se il nodo da eliminare è l'ultimo della lista entro nell'ELSE*/
            else{
                temp->prev->next = NULL;
            }
            /*aggiorno matrice feromone*/
            game_matrix[(temp->x*matrix_lenght)+temp->y] -= pheromone_unit;
            break;
        }
    }

    /* INVIO LE INFO DEL PLAYER/NODO DA ELIMINARE A TUTTI GLI ALTRI NODI*/
    players_struct = &players_root;
    while (players_struct->next != NULL){
        players_struct = players_struct->next;
        int temp_gate = players_struct->ID;

        cMessage *deletePlayer = new cMessage("deletePlayer");
        deletePlayer->setKind(id_to_delete);
        send(deletePlayer, "out", temp_gate);

    }
}

/*  DEFINIZIONE DELLE 8 POSSIBILI MOSSE con X che rappresenta il player
 *
 *  0   1   2
 *  7   X   3
 *  6   5   4
 */
void Server::move(int mossa, int* x, int* y){
    game_matrix[((*x)*matrix_lenght)+(*y)] -= pheromone_unit;
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
        default: break;
    }
    game_matrix[((*x)*matrix_lenght)+(*y)] += pheromone_unit;
}

int Server::evaluateReckoningStep(int x, int y, int game_matrix_for_session[]){
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
        else if (desired_pos[1] == y){
            no_moviment++;
            mossa = -1;
        }
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

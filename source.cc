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

#include "source.h"

Define_Module(Source);

void Source::initialize()
{
    maxJobsTime = par("maxJobsTime");
    interArrival = (getSystemModule()->par("game_cycle").doubleValue());

    /*RANDOM viene creato con un seme casuale, non riproducibile*/
    unsigned seed = std::chrono::system_clock::now().time_since_epoch().count();
    random = std::mt19937_64(seed);

    /*Imposto seme per il RANDOM_FIX che è un seme fissato e RIPRODUCIBILE*/
    random_fix = this->getRNG(0);

    // schedule the first message timer for start time
    scheduleAt(simTime()+interArrival, new cMessage("newJobTimer"));
}

void Source::handleMessage(cMessage *msg)
{
    if(msg->isSelfMessage()){
        // reschedule the timer for the next message
        scheduleAt(simTime() + interArrival, msg);

        /* in "n_time" inserisco il numero random di job che devono essere erogati
         * uso "maxJoibsTime +1" perchè il numero di job può essere 0..1..2..maxJoibsTime
         */
        int n_time = random_fix->intRand() % (maxJobsTime+1);   //con seme fissato
        /*int n_time = random() % (maxJobsTime+1);*/    //con seme casuale

        for(int i=0;i<n_time;i++){
            /* Definisco il nome del mess da creare */
            char id_job[10];
            sprintf (id_job, "job %d", i);
            /*itoa(i, id_job, 10);
            char job_text[10] = "job ";*/
            /**/
            cMessage *new_job = new cMessage(id_job);
            send(new_job, "out");
        }
    }
}

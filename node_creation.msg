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

//
// TODO generated message class
//
//PROBLEMA DA RISOLVERE --> RISOLTO! 
//uso array multidimensionali memorizzati su un array mono dimensionale

cplusplus {{
#include "server.h"
}};

struct Server::player;

message Node_creation {
  	Server::player player_data;
    int ID;
    int matrix_lenght;
    int tolerance;
    int pheromone;
}

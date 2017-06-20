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

package deadreckoning;

//
// TODO auto-generated module
//
simple Server
{
    parameters:
        int node_number;
        int node_lifecycle @unit(s);
        int game_matrix;
        int tolerance;
        int pheromone_unit = default(1);
        int info_cycle = default(10);
        @signal[positionFixSignal](type="long");
        @statistic[positionFixSignal](title="num. of dead reckoning position fix";record=vector,max);
        @signal[positionFixSignalXcycle](type="long");
        @statistic[positionFixSignalXcycle](title="num. of dead reckoning position fix cycle";record=vector,mean,max);
        @signal[free](type="bool");
        @statistic[free](title="time without player";record=vector,timeavg);
        @signal[activeNodeSignal](type="long");
        @statistic[activeNodeSignal](title="num. of active player every cycle";record=vector,mean,max);
        @signal[rejectNodeSignal](type="long");
        @statistic[rejectNodeSignal](title="num. of reject player every cycle";record=vector,mean,max);
        @signal[no_playerMoviment](type="long");
        @statistic[no_playerMoviment](title="num. of times that dead reckoning for players choose to stay in their position";record=vector,max);
    gates:
        input in[];
        output out[];
}

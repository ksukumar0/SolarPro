/*
   Wireless Sensor Networks Laboratory 2019 -- Group 1

   Technische Universität München
   Lehrstuhl für Kommunikationsnetze
http://www.lkn.ei.tum.de

copyright (c) 2019 Chair of Communication Networks, TUM

project: SolarPro

contributors:
 * Karthik Sukumar
 * Johannes Machleid

 This c-file is designed for all nodes to set specific Node IDs depending on the RIME ID.
 */


// Std file includes
#include <stdio.h>

// Contiki includes
#include "core/net/linkaddr.h"

// Private includes
#include "nodeID.h"


static const nodeID_t nodes[] =
{
    {1,2048,0xBFED},
    {2,2215,0xF2F3},
    {3,2162,0x60EF},
    {4,2031,0xE7F3},
    {5,1982,0xDCF3},
    {6,2053,0xDDED},
    {7,2207,0xB0EE},
    {8,2071,0xB964}
};

const uint32_t NETWORKSIZE = (sizeof(nodes))/(sizeof(nodeID_t));

/*** This prints the Network IDs of the nodes ***/
//function for printing the node IDs on the console
//@return int: actual direction of the joystick beeing pressed.
void print_node_IDs( void )
{
    int i=0;
    printf("=========================================\n");
    for (i=0;i<NETWORKSIZE;i++)
    {
        printf("Node ID: %d, Serial No: %d, Rime ID: %x\n",nodes[i].nodeID,nodes[i].serialNumber,nodes[i].rimeID);
    }
    printf("=========================================\n");
}

node_num_t getMyNodeID( linkaddr_t l )
{
    int i=0;
    for (i=0;i<NETWORKSIZE;i++)
    {
        if ( l.u16 == nodes[i].rimeID )
        {
            return nodes[i].nodeID;
        }
    }
    return -1;
}

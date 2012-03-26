/*******************************************************************************

  Eurecom OpenAirInterface
  Copyright(c) 1999 - 2011 Eurecom

  This program is free software; you can redistribute it and/or modify it
  under the terms and conditions of the GNU General Public License,
  version 2, as published by the Free Software Foundation.

  This program is distributed in the hope it will be useful, but WITHOUT
  ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
  FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
  more details.

  You should have received a copy of the GNU General Public License along with
  this program; if not, write to the Free Software Foundation, Inc.,
  51 Franklin St - Fifth Floor, Boston, MA 02110-1301 USA.

  The full GNU General Public License is included in this distribution in
  the file called "COPYING".

  Contact Information
  Openair Admin: openair_admin@eurecom.fr
  Openair Tech : openair_tech@eurecom.fr
  Forums       : http://forums.eurecom.fsr/openairinterface
  Address      : Eurecom, 2229, route des crêtes, 06560 Valbonne Sophia Antipolis, France

*******************************************************************************/

/*! \file sumo.c
* \brief The OMG interface to SUMO (Simulation of Urban Mobility)
* \author  S. Uppoor, J. Harri
* \date 2012
* \version 0.1
* \company INRIA, Eurecom
* \email: sandesh.uppor@inria.fr, haerri@eurecom.fr
* \note
* \warning
*/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include <time.h>
#include <math.h>
#include <string.h>

#include "sumo.h"

#include "client_traci_OMG.h"


int start_sumo_generator(omg_global_param omg_param_list) {

  char sumo_line[300];

//  sprintf(sumo_line, " -c %s  -b %d -e %d --remote-port %d --step-length %d -v ",omg_param_list.sumo_config, omg_param_list.sumo_start, omg_param_list.sumo_end, omg_param_list.sumo_port, omg_param_list.sumo_step);

// sprintf(sumo_line, "%s -c %s  -b %d -e %d --remote-port %d --step-length %d -v ",omg_param_list.sumo_command, omg_param_list.sumo_config, omg_param_list.sumo_start, omg_param_list.sumo_end, omg_param_list.sumo_port, omg_param_list.sumo_step);

sprintf(sumo_line, "%s -c %s ",omg_param_list.sumo_command, omg_param_list.sumo_config);

  printf("%s\n",sumo_line);
//  char *sumo_arg[] = { NULL, sumo_line, NULL };
//  char *sumo_env[] = { NULL };

 // if ( (pid = fork()) == 0)
 // {
    // Start SUMO in the child process
   //execve(omg_param_list.sumo_command, sumo_arg, sumo_env);// starts SUMO in a child process
 //  system(sumo_line);
   //childs addresss space
 // }
 
  //still in the parent process
 
  // Talk to SUMO 

  targetTime = 1;
  
// switch on error to return to OAI
  int error = handshake(omg_param_list.sumo_host,omg_param_list.sumo_port);
  
  init(omg_param_list.sumo_end - omg_param_list.sumo_start);
  
  int max_node_SUMO = 200; //commandGetMaxSUMONodesVariable(); JHNOTE: method not implemented in TraCI server

  printf("received Number of nodes %d\n", max_node_SUMO);

  // create the OAI/SUMO ID manager
  id_manager = create_IDManager();

  printf(" ID Manager Created \n");
  NodePtr node = NULL;
  MobilityPtr mobility = NULL;
  
  active_nodes = NULL;  // container to return a subset of only ACTIVE OAI nodes in SUMO
  
  departed  = NULL;
  arrived = NULL;

  last_update_time = 0.0;

  // just check for faulty values
  if (omg_param_list.nodes <= 0){
    printf("Number of nodes has not been set\n");
    //LOG_W(OMG, "Number of nodes has not been set\n");
    return(-1);
  }
  printf("Number of OAI-equipped nodes in SUMO has been set to %d\n", omg_param_list.nodes);
  printf("Number of SUMO simulated nodes has been set to %d\n", max_node_SUMO);
  //LOG_I(OMG, "Number of OAI-equipped nodes in SUMO has been set to %d\n", omg_param_list.nodes);
  //LOG_I(OMG, "Number of SUMO simulated nodes has been set to %d\n", max_node_SUMO);
  // check and match number of nodes in mobility file provided
  if (omg_param_list.nodes > max_node_SUMO){
    printf("Not all OAI nodes will be moving according to SUMO.\n");
    //LOG_I(OMG, "Not all OAI nodes will be moving according to SUMO.\n");
  }
  else {
    printf("OAI nodes will be mapped to a subset of SUMO nodes\n");
    //LOG_I(OMG, "OAI nodes will be mapped to a subset of SUMO nodes\n");
  }

  if (omg_param_list.nodes_type == eNB) {
    LOG_I(OMG, "Node type has been set to eNB\n");
  } 
  else if (omg_param_list.nodes_type == UE) {
    printf("Node type has been set to UE\n");
    //LOG_I(OMG, "Node type has been set to UE\n");
  }
  
  int n_id = 0;
  for ( n_id = 0; n_id< omg_param_list.nodes; n_id++) {
    
    node = (NodePtr) create_node();
    mobility = (MobilityPtr) create_mobility();
    node->mobile = 0; // 0 means inactive in SUMO; 1 means active in SUMO; as long as a mapping between OAI-SUMO has not been created, nodes are inactive and do not move
    node->ID = n_id; // this is OAI ID, not SUMO
    node->type = omg_param_list.nodes_type; // UE eNB...
    node->generator = omg_param_list.mobility_type; // SUMO
    
    Node_Vector[SUMO] = (Node_list) add_entry(node, Node_Vector[SUMO]);     
  }
  
  update_IDs();  // update the mapping between departed and arrived nodes in SUMO.

  return(0);
}

void update_IDs() {
  printf("Updating the ID mapping between SUMO and OAI\n");
  //LOG_D(OMG, "Updating the ID mapping between SUMO and OAI\n"); 

  String_list tmp_arrived, tmp_departed;
  
  if(arrived == NULL) {
     printf("arrived vehicles is NULL \n"); 
     return;
  }

  tmp_arrived = arrived;
  tmp_departed = departed;

  if(tmp_arrived->string !=NULL) {
   
    char *tmp_string = tmp_arrived->string;
     printf("1 head is not null and value is: %s\n",tmp_string);
    if(!desactivate_and_unmap(tmp_string)) {
      printf("Could not locate the OAI node ID %s \n", tmp_string);
      //LOG_I(OMG, "Could not locate the OAI node ID %s \n", tmp_string);
    }
    
  }
  while (tmp_arrived->next != NULL) {
    printf("1 main is not null \n");
    char *tmp_string = tmp_arrived->string;
    if(!desactivate_and_unmap(tmp_string)) {
       printf("Could not locate the OAI node\n");
      // LOG_I(OMG, "Could not locate the OAI node\n");
    }
    tmp_arrived = tmp_arrived->next;  
  }
  arrived = clear_String_list(arrived);

  if(departed == NULL) {
     printf("departed vehicles is NULL \n"); 
     return;
  }

  if(tmp_departed->string !=NULL) {
    printf("2 head is not null \n");
    char tmp_string [strlen(tmp_departed->string)];     
    strcpy(tmp_string, tmp_departed->string);
    printf("2 head is not null and value is: %s\n",tmp_string);
    //char *tmp_string = tmp_departed->string);
    int OAI_ID = get_oaiID_by_SUMO(tmp_string, id_manager);
    if (OAI_ID ==-1) {
      if (!activate_and_map(tmp_string)) {
	  printf("Reached the Maximum of OAI nodes to be mapped to SUMO\n");
         // LOG_I(OMG, "Reached the Maximum of OAI nodes to be mapped to SUMO\n");
        return;  // stopping mapping as the maximum of OAI nodes has been reached;
      }
    }
  }
  while (tmp_departed->next != NULL) {
    printf("2 main is not null \n");
    char tmp_string [strlen(tmp_departed->string)];
    strcpy(tmp_string, tmp_departed->string);
    //char *tmp_string = tmp_departed->string;
    int OAI_ID = get_oaiID_by_SUMO(tmp_string, id_manager);
    if (OAI_ID ==-1) {
      if (!activate_and_map(tmp_string)) {
        printf("Reached the Maximum of OAI nodes to be mapped to SUMO\n");
        //LOG_I(OMG, "Reached the Maximum of OAI nodes to be mapped to SUMO\n");
        return;  // stopping mapping as the maximum of OAI nodes has been reached;
      }  
    }
    tmp_departed = tmp_departed->next;
  }

   Map_list tmp =  id_manager->map_oai2sumo;
     char *sumo_id = get_sumo_entry(0, tmp);
  
     if(sumo_id !=NULL) {
          printf(" 2: ACTIVATE and MAP: OAI ID is: %d and SUMO ID is %s \n",0, sumo_id);  
      } 
  departed = clear_String_list(departed);
}

bool desactivate_and_unmap(char *sumo_id) {
  int OAI_ID = remove_oaiID_by_SUMO(sumo_id, id_manager);
  if (OAI_ID !=-1) {
      NodePtr node = find_node(Node_Vector[SUMO], OAI_ID, SUMO);
      node->mobile = 0; // this node is now inactive;
      return true;
  }
  else {
      printf("Could not desactive an OAI node, as the SUMO-OAI mapping could not be found for the SUMO node ID %s \n", sumo_id);
     // LOG_D(OMG, "Could not desactive an OAI node, as the SUMO-OAI mapping could not be found for the SUMO node ID %s \n", sumo_id); 
      return false;
  }
}


bool activate_and_map(char *sumo_id) {
  MapPtr map = create_map();
  MapPtr map2 = create_map();

  NodePtr active_node = get_first_inactive_OAI_node(Node_Vector[SUMO], SUMO);
  if(active_node != NULL) {  // found an inactive OAI node; will be mapped to SUMO
    active_node->mobile = 1; // now node is active in SUMO
 
    char * copy_sumo_id = malloc(sizeof((int)strlen(sumo_id)));
    copy_sumo_id = strcpy( copy_sumo_id, sumo_id);
		
    copy_sumo_id = sumo_id;
    map->oai_id = active_node->ID;
    map->sumo_id = sumo_id;//sumo_id;
     
    map2->oai_id = active_node->ID;
    map2->sumo_id = copy_sumo_id;//sumo_id;	
    
    printf("added a mapping between oai ID:  %d and SUMO ID: %s \n",map->oai_id, map->sumo_id); 
    id_manager->map_sumo2oai = add_map_entry(map, id_manager->map_sumo2oai);

    
    printf("added a new sumo2oai entry\n");
    if (id_manager->map_oai2sumo == NULL) {
       printf("uninitialized oai2sumo map\n");
    }
    id_manager->map_oai2sumo = add_map_entry(map2, id_manager->map_oai2sumo); //map_sumo2oai
	printf("added a new oai2sumo entry\n");

     Map_list tmp =  id_manager->map_oai2sumo;
     char *sumo_id = get_sumo_entry(active_node->ID, tmp);
  
     if(sumo_id !=NULL) {
          printf(" ACTIVATE and MAP: OAI ID is: %d and SUMO ID is %s \n",active_node->ID, sumo_id);  
      }   
  return true;
  }
  else {
    printf("All OAI Nodes are already active in SUMO; cannot control this SUMO node in OAI\n");
    //LOG_D(OMG, "All OAI Nodes are already active in SUMO; cannot control this SUMO node in OAI\n"); 
    return false;
  }
}  


void update_sumo_nodes(double cur_time) {
   if((cur_time - last_update_time) < MIN_SUMO_STEP)   // the min time interval for SUMO must be 100ms or more
      return;
	
   last_update_time = cur_time;  // keeps track of the last update time to get the update interval

   commandSimulationStep(1.0);// Advance the SUMO simulation by cur_time units

   update_IDs();  // both are in the  traCI client

}

void update_sumo_positions(NodePtr node){

  //LOG_D(OMG, "--------GET SUMO Mobility for a single node--------\n");
  printf("--------GET SUMO Mobility for a single node--------\n");
  //vehicleVar* next_loc = NULL;
  
  //char *sumo_id = get_SumoID_by_OAI(node->ID, id_manager);  
  Map_list tmp =  id_manager->map_oai2sumo;
  char *sumo_id = get_sumo_entry(node->ID, tmp);
  
  if(sumo_id !=NULL) {   
    printf(" OAI ID is: %d and SUMO ID is %s \n",node->ID, sumo_id);    
    GetPosition(node, sumo_id);
    GetSpeed(node, sumo_id);
  }
  /*next_loc = get_pos_speed(node->ID, 2);// number 2 reprsents getVehicleVariable should be called twice for speed and loc
  
  node->X_pos = next_loc->x;
  node->Y_pos = next_loc->y;
  node->mob->speed = next_loc->speed;*/
}

Node_list get_sumo_positions_updated(double cur_time) {
  
  //LOG_D(OMG, "--------GET SUMO Mobility for a group of ACTIVE OAI nodes--------\n");
  printf("--------GET SUMO Mobility for a group of ACTIVE OAI nodes--------\n");
 // reset_node_list(active_nodes);

  if (Node_Vector[SUMO] != NULL){
    Node_list tmp = Node_Vector[SUMO];
    while (tmp != NULL){
      printf("Non NULL node in Node_Vector...is node ID: %d it active? with mobility %d\n", tmp->node->ID, tmp->node->mobile);
      if ((tmp->node->generator == SUMO) && (tmp->node->mobile == 1)) {  // OAI node MUST be active
        //LOG_T(OMG, "found an active node with id %d \n", tmp->node->ID);
	//LOG_D(OMG, "Old Positions \n");		
	printf("Old Positions \n");
        display_node_position(tmp->node->ID, tmp->node->generator, tmp->node->type ,tmp->node->mobile, tmp->node->X_pos, tmp->node->Y_pos );
        
        update_sumo_positions(tmp->node);
	
        printf("New Positions \n");
        //LOG_D(OMG, "New Positions \n");		
	display_node_position(tmp->node->ID, tmp->node->generator, tmp->node->type ,tmp->node->mobile, tmp->node->X_pos, tmp->node->Y_pos );
	active_nodes = add_entry(tmp->node, active_nodes);
      }
      tmp = tmp->next;
    }
  }
  return active_nodes;
}

NodePtr get_first_inactive_OAI_node(Node_list list, int node_type) {
  Node_list  current;

  if (list !=NULL) {                             //start search
    current = list;
    while (current->next != NULL) {
      if(current->node->mobile == 0) {
        return current->node;
      }
      current = current->next;
    }
  }
  return NULL;  // all nodes are active already..reached the maximum number of OAI nodes 
}

bool stop_sumo_generator() {

  commandClose(); // closing the connection with SUMO via TraCI
  close_connection(); // closing the socket connection
  kill(pid); // killing SUMO in case it could not close by itself
  return true;
}

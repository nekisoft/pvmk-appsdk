/*
    Copyright (C) 2001 Hotwarez LLC, Goldtree Enterprises
  
    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Library General Public
    License as published by the Free Software Foundation; 
    version 2 of the License.
  
    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Library General Public License for more details.
  
    You should have received a copy of the GNU Library General Public
    License along with this library; if not, write to the Free
    Software Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
*/

#include <stdlib.h>
#include <string.h>
#include "util.h"  /* need exit_gracefully */
#include "keys.h"  /* need Get_Keypress() */
#include "tanks.h"
#include "object.h"
#include "collide.h"

extern char g_DataPath[255];

void Load_Tank( Vehicle *tank, enum VehicleType tank_type )
{
	FILE *fp;
	char newfilename[512];
	sprintf(newfilename,"%sgamedata/tanks.tbf",g_DataPath);

	if( (fp = fopen( newfilename, "rb" )) == NULL )
	{
		printf( "Load_Tank() : fopen failed\n" );
		Get_Keypress();
		exit_gracefully();
	}

	fseek( fp, 316 * tank_type, SEEK_SET );

	//fucking kill me, we can't depend on having the struct line up
	memset(tank, 0, sizeof(*tank));
	
	fread(&(tank->vtype), 1, 4, fp);
	fread(&(tank->team), 1, 4, fp);
	fread(&(tank->vehicle_mode), 1, 4, fp);
	fread(&(tank->alive), 1, 4, fp);
	fread(&(tank->surface_rad), 1, 4, fp);
	
	fread(&(tank->obj), 1, 4, fp);
	fread(&(tank->world_obj), 1, 4, fp);
	fread(&(tank->collision_obj), 1, 4, fp);
	fread(&(tank->world_collision_obj), 1, 4, fp);
	
	fread(&(tank->box.min_x), 1, 4, fp);
	fread(&(tank->box.max_x), 1, 4, fp);
	fread(&(tank->box.min_y), 1, 4, fp);
	fread(&(tank->box.max_y), 1, 4, fp);
	fread(&(tank->box.min_z), 1, 4, fp);
	fread(&(tank->box.max_z), 1, 4, fp);
	
	fread(&(tank->mbox.min_x), 1, 4, fp);
	fread(&(tank->mbox.max_x), 1, 4, fp);
	fread(&(tank->mbox.min_y), 1, 4, fp);
	fread(&(tank->mbox.max_y), 1, 4, fp);
	fread(&(tank->mbox.min_z), 1, 4, fp);
	fread(&(tank->mbox.max_z), 1, 4, fp);
	
	fread(&(tank->collision_edges), 1, 8, fp);
	fread(&(tank->orient), 1, 36, fp);
	
	fread(&(tank->vel[0]), 1, 4, fp);
	fread(&(tank->vel[1]), 1, 4, fp);
	fread(&(tank->vel[2]), 1, 4, fp);
	
	fread(&(tank->air_forward_speed), 1, 4, fp);
	fread(&(tank->air_max_forward_speed), 1, 4, fp);
	fread(&(tank->air_inc_forward_speed), 1, 4, fp);
	fread(&(tank->air_max_sidestep_speed), 1, 4, fp);
	fread(&(tank->air_inc_sidestep_speed), 1, 4, fp);
	
	fread(&(tank->air_rise_rot_speed), 1, 4, fp);
	fread(&(tank->air_spin_rot_speed), 1, 4, fp);
	fread(&(tank->air_inc_rot_speed), 1, 4, fp);
	fread(&(tank->air_max_rot_speed), 1, 4, fp);
	
	fread(&(tank->surface_max_speed), 1, 4, fp);
	fread(&(tank->surface_inc_speed), 1, 4, fp);
	fread(&(tank->surface_inc_sidestep_speed), 1, 4, fp);
	
	fread(&(tank->surface_rot_speed), 1, 4, fp);
	fread(&(tank->surface_inc_rot_speed), 1, 4, fp);
	fread(&(tank->surface_max_rot_speed), 1, 4, fp);
	
	fread(&(tank->target), 1, 4, fp);
	
	fread(&(tank->laser_speed), 1, 4, fp);
	fread(&(tank->laser_life), 1, 2, fp);
	fread(&(tank->laser_damage), 1, 2, fp);
	fread(&(tank->laser_reload_time), 1, 2, fp);
	fread(&(tank->frames_till_fire_laser), 1, 2, fp);
	
	fread(&(tank->missile_speed), 1, 4, fp);
	fread(&(tank->turning_angle), 1, 4, fp);
	fread(&(tank->missile_life), 1, 2, fp);
	fread(&(tank->missile_damage), 1, 2, fp);
	fread(&(tank->missile_reload_time), 1, 2, fp);
	fread(&(tank->frames_till_fire_missile), 1, 2, fp);
	fread(&(tank->missile_generation_time), 1, 2, fp);
	fread(&(tank->frames_till_new_missile), 1, 2, fp);
	fread(&(tank->max_missile_storage), 1, 2, fp);
	fread(&(tank->missiles_stored), 1, 2, fp);
	
	fread(&(tank->max_projectiles), 1, 2, fp);
	fread(&(tank->projectile_list), 1, 4, fp);

	fread(&(tank->max_hitpoints), 1, 4, fp);
	fread(&(tank->current_hitpoints), 1, 4, fp);
	
	fread(&(tank->ramming_active), 1, 2, fp);
	fread(&(tank->ramming_damage), 1, 2, fp);
	
	fread(&(tank->double_lasers_active), 1, 2, fp);
	
	fread(&(tank->mine_reload_time), 1, 2, fp);
	fread(&(tank->mine_damage), 1, 2, fp);
	fread(&(tank->mine_life), 1, 2, fp);
	
	fread(&(tank->cs_missile_reload_time), 1, 2, fp);
	fread(&(tank->cs_missile_life), 1, 2, fp);
	fread(&(tank->cs_missile_speed), 1, 4, fp);
	
	fread(&(tank->controls_scrambled), 1, 2, fp);
	fread(&(tank->frames_till_unscramble), 1, 2, fp);
	fread(&(tank->scramble_life), 1, 2, fp);
	
	fread(&(tank->traitor_missile_reload_time), 1, 2, fp);
	fread(&(tank->traitor_missile_life), 1, 2, fp);
	fread(&(tank->traitor_missile_speed), 1, 4, fp);
	
	fread(&(tank->traitor_life), 1, 2, fp);
	fread(&(tank->traitor_active), 1, 2, fp);
	fread(&(tank->frames_till_traitor_deactivate), 1, 2, fp);
	
	fread(&(tank->anti_missile_active), 1, 2, fp);
	
	fread(&(tank->cloaking_active), 1, 2, fp);
	fread(&(tank->cloak_reload_time), 1, 2, fp);
	fread(&(tank->frames_till_cloak), 1, 2, fp);
	fread(&(tank->cloak_time), 1, 2, fp);
	fread(&(tank->frames_till_cloak_suck), 1, 2, fp);
	
	fread(&(tank->decoy_life), 1, 2, fp);
	fread(&(tank->decoy_reload_time), 1, 2, fp);

	fclose( fp );
	
} /* End of Load_Tank */

void init_vehicle_gradient( Vehicle *tank )
{
    long i;

    if( tank->team == RED_TEAM ) {

        for( i = 0; i < tank->obj->faces; i++ ) {
            if( tank->obj->face[i].gradient == 0 )
                tank->obj->face[i].gradient = RedVehicleFirstGrad;
            else if( tank->obj->face[i].gradient == 1 )
                tank->obj->face[i].gradient = RedVehicleSecondGrad;
            else if( tank->obj->face[i].gradient == 2 )
                tank->obj->face[i].gradient = RedVehicleThirdGrad;
            else
                tank->obj->face[i].gradient = RedVehicleFirstGrad;
        }

        for( i = 0; i < tank->world_obj->faces; i++ ) {
            if( tank->world_obj->face[i].gradient == 0 )
                tank->world_obj->face[i].gradient = RedVehicleFirstGrad;
            else if( tank->world_obj->face[i].gradient == 1 )
                tank->world_obj->face[i].gradient = RedVehicleSecondGrad;
            else if( tank->world_obj->face[i].gradient == 2 )
                tank->world_obj->face[i].gradient = RedVehicleThirdGrad;
            else
                tank->world_obj->face[i].gradient = RedVehicleFirstGrad;
        }

    }
    else {

        for( i = 0; i < tank->obj->faces; i++ ) {
            if( tank->obj->face[i].gradient == 0 )
                tank->obj->face[i].gradient = BlueVehicleFirstGrad;
            else if( tank->obj->face[i].gradient == 1 )
                tank->obj->face[i].gradient = BlueVehicleSecondGrad;
            else if( tank->obj->face[i].gradient == 2 )
                tank->obj->face[i].gradient = BlueVehicleThirdGrad;
            else
                tank->obj->face[i].gradient = BlueVehicleFirstGrad;
        }

        for( i = 0; i < tank->world_obj->faces; i++ ) {
            if( tank->world_obj->face[i].gradient == 0 )
                tank->world_obj->face[i].gradient = BlueVehicleFirstGrad;
            else if( tank->world_obj->face[i].gradient == 1 )
                tank->world_obj->face[i].gradient = BlueVehicleSecondGrad;
            else if( tank->world_obj->face[i].gradient == 2 )
                tank->world_obj->face[i].gradient = BlueVehicleThirdGrad;
            else
                tank->world_obj->face[i].gradient = BlueVehicleFirstGrad;
        }
    }
}



void Init_Tank( Vehicle *tank, enum VehicleType tank_type, team_type team )
    {

    tank->vtype = tank_type;
    tank->team = team;

    if( (tank->obj = (PointFace *) malloc( sizeof( PointFace ) ) ) == NULL ) {
        printf("Init_Tank() : malloc failed\n");
        Get_Keypress();
        exit_gracefully();
    }
    tank->obj->point = NULL;
    tank->obj->points = 0;
    tank->obj->face = NULL;
    tank->obj->faces = 0;

    if( (tank->collision_obj = (PointFace *) malloc( sizeof( PointFace ) ) ) == NULL ) {
        printf("Init_Tank() : malloc failed\n");
        Get_Keypress();
        exit_gracefully();
    }
    tank->collision_obj->point = NULL;
    tank->collision_obj->points = 0;
    tank->collision_obj->face = NULL;
    tank->collision_obj->faces = 0;

    switch( tank_type )
        {
         case Wasp :
             get_object( tank->obj, "3d_data/wasp.pfd" );
             get_object( tank->collision_obj, "3d_data/cwasp.pfd" );
             break;
         case Beetle :
             get_object( tank->obj, "3d_data/beetle.pfd" );
             get_object( tank->collision_obj, "3d_data/cbeetle.pfd" );
             break;
         case Flea :
             get_object( tank->obj, "3d_data/flea.pfd" );
             get_object( tank->collision_obj, "3d_data/cflea.pfd" );
             break;
         case Mosquito :
             get_object( tank->obj, "3d_data/mosquito.pfd" );
             get_object( tank->collision_obj, "3d_data/cmosquit.pfd" );
             break;
         case Spider :
             get_object( tank->obj, "3d_data/spider.pfd" );
             get_object( tank->collision_obj, "3d_data/cspider.pfd" );
             break;
         case Dragonfly :
             get_object( tank->obj, "3d_data/dragon.pfd" );
             get_object( tank->collision_obj, "3d_data/cdragon.pfd" );
             break;
         case Roach :
             get_object( tank->obj, "3d_data/roach.pfd" );
             get_object( tank->collision_obj, "3d_data/croach.pfd" );
             break;
         case Locust :
             get_object( tank->obj, "3d_data/locust.pfd" );
             get_object( tank->collision_obj, "3d_data/clocust.pfd" );
             break;

         default :
             printf("Invalid tank number in Init_Tank \n");
             Get_Keypress();
             exit_gracefully();
             break;

        } /* End switch */

    if( ( tank->world_obj = (PointFace *) malloc( sizeof( PointFace ) ) ) == NULL ) {
        printf("Init_Tank() : malloc failed\n");
        Get_Keypress();
        exit_gracefully();
    }
    tank->world_obj->point = NULL;
    tank->world_obj->points = 0;
    tank->world_obj->face = NULL;
    tank->world_obj->faces = 0;

    init_world_obj( tank );

    init_vehicle_gradient( tank );

    if( ( tank->world_collision_obj = (PointFace *) malloc( sizeof( PointFace ) ) ) == NULL ) {
        printf("Init_Tank() : malloc failed\n");
        Get_Keypress();
        exit_gracefully();
    }
    tank->world_collision_obj->point = NULL;
    tank->world_collision_obj->points = 0;
    tank->world_collision_obj->face = NULL;
    tank->world_collision_obj->faces = 0;

    init_world_collision_obj( tank );

    init_bounding_box( tank );

    tank->collision_edges.edge = NULL;
    tank->collision_edges.edges = 0;

    init_edge_table( tank );

    tank->alive = TRUE;
    tank->target = NULL;
    tank->vehicle_mode = Surface;

} /* End of Init_Tank */


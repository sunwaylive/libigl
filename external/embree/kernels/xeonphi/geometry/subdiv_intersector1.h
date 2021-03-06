// ======================================================================== //
// Copyright 2009-2014 Intel Corporation                                    //
//                                                                          //
// Licensed under the Apache License, Version 2.0 (the "License");          //
// you may not use this file except in compliance with the License.         //
// You may obtain a copy of the License at                                  //
//                                                                          //
//     http://www.apache.org/licenses/LICENSE-2.0                           //
//                                                                          //
// Unless required by applicable law or agreed to in writing, software      //
// distributed under the License is distributed on an "AS IS" BASIS,        //
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. //
// See the License for the specific language governing permissions and      //
// limitations under the License.                                           //
// ======================================================================== //

#pragma once

#include "subdivpatch1.h"
#include "common/ray.h"
#include "geometry/filter.h"

namespace embree
{

  static __forceinline void intersect1_tri16_precise(const mic_f &dir_xyz,
						     const mic_f &org_xyz,
						     Ray& ray,
						     const mic3f &v0_org,
						     const mic3f &v1_org,
						     const mic3f &v2_org,
						     const mic_f &u_grid,
						     const mic_f &v_grid,
						     const unsigned int offset_v0,
						     const unsigned int offset_v1,
						     const unsigned int offset_v2,
						     const mic_m &m_active,
						     const unsigned int subdiv_patch_index)
  {
    const mic3f ray_dir(swAAAA(dir_xyz),swBBBB(dir_xyz),swCCCC(dir_xyz));
    
    const mic3f v0 = v0_org; // - ray_org;
    const mic3f v1 = v1_org; // - ray_org;
    const mic3f v2 = v2_org; // - ray_org;
   
    const mic3f e0 = v2 - v0;
    const mic3f e1 = v0 - v1;	     
    const mic3f e2 = v1 - v2;	     

    const mic3f Ng1     = cross(e1,e0);
    const mic3f Ng      = Ng1+Ng1;
    const mic_f den     = dot(Ng,ray_dir);	      
    const mic_f rcp_den = rcp(den);

#if defined(RTCORE_BACKFACE_CULLING)
    mic_m m_valid = m_active & (den > zero);
#else
    mic_m m_valid = m_active;
#endif

    const mic_f u = dot(cross(v2+v0,e0),ray_dir) * rcp_den; 
    m_valid       = ge( m_valid, u, zero);

    const mic_f v       = dot(cross(v0+v1,e1),ray_dir) * rcp_den; 
    m_valid       = ge( m_valid, v, zero);

    const mic_f w       = dot(cross(v1+v2,e2),ray_dir) * rcp_den;  
    m_valid       = ge( m_valid, w, zero);

    if (unlikely(none(m_valid))) return;
    
    const mic_m m_den = ne(m_valid,den,zero);
    const mic_f t = dot(v0,Ng) * rcp_den;
    mic_f max_dist_xyz = mic_f(ray.tfar);
    mic_m m_final      = lt(lt(m_den,mic_f(ray.tnear),t),t,max_dist_xyz);

    //////////////////////////////////////////////////////////////////////////////////////////////////

    if (unlikely(any(m_final)))
      {
	STAT3(normal.trav_prim_hits,1,1,1);
	max_dist_xyz  = select(m_final,t,max_dist_xyz);
	const mic_f min_dist = vreduce_min(max_dist_xyz);
	const mic_m m_dist = eq(min_dist,max_dist_xyz);
	const size_t index = bitscan(toInt(m_dist));

	const mic_f u0 = uload16f_low(&u_grid[offset_v0]);
	const mic_f u1 = uload16f_low(&u_grid[offset_v1]);
	const mic_f u2 = uload16f_low(&u_grid[offset_v2]);
	const mic_f u_final = u * u1 + v * u2 + (1.0f-u-v) * u0;

	const mic_f v0 = uload16f_low(&v_grid[offset_v0]);
	const mic_f v1 = uload16f_low(&v_grid[offset_v1]);
	const mic_f v2 = uload16f_low(&v_grid[offset_v2]);
	const mic_f v_final = u * v1 + v * v2 + (1.0f-u-v) * v0;

	//const mic_f u_final = u;
	//const mic_f v_final = v;

	const mic_m m_tri = m_dist^(m_dist & (mic_m)((unsigned int)m_dist - 1));
                
	assert( countbits(m_tri) == 1);

	const mic_f gnormalx(Ng.x[index]);
	const mic_f gnormaly(Ng.y[index]);
	const mic_f gnormalz(Ng.z[index]);
		  
	compactustore16f_low(m_tri,&ray.tfar,min_dist);
	compactustore16f_low(m_tri,&ray.u,u_final); 
	compactustore16f_low(m_tri,&ray.v,v_final); 
	compactustore16f_low(m_tri,&ray.Ng.x,gnormalx); 
	compactustore16f_low(m_tri,&ray.Ng.y,gnormaly); 
	compactustore16f_low(m_tri,&ray.Ng.z,gnormalz); 

	ray.geomID = -1;
	ray.primID = subdiv_patch_index;      
      }
  };


  static __forceinline bool occluded1_tri16_precise( const mic_f &dir_xyz,
						     const mic_f &org_xyz,
						     Ray& ray,
						     const mic3f &v0_org,
						     const mic3f &v1_org,
						     const mic3f &v2_org,
						     const mic_f &u_grid,
						     const mic_f &v_grid,
						     const unsigned int offset_v0,
						     const unsigned int offset_v1,
						     const unsigned int offset_v2,
						     const mic_m &m_active,
						     const unsigned int subdiv_patch_index)
  {
    const mic3f ray_dir(swAAAA(dir_xyz),swBBBB(dir_xyz),swCCCC(dir_xyz));
    
    const mic3f v0 = v0_org; // - ray_org;
    const mic3f v1 = v1_org; // - ray_org;
    const mic3f v2 = v2_org; // - ray_org;
   
    const mic3f e0 = v2 - v0;
    const mic3f e1 = v0 - v1;	     
    const mic3f e2 = v1 - v2;	     

    const mic3f Ng1     = cross(e1,e0);
    const mic3f Ng      = Ng1+Ng1;
    const mic_f den     = dot(Ng,ray_dir);	      
    const mic_f rcp_den = rcp(den);

#if defined(RTCORE_BACKFACE_CULLING)
    mic_m m_valid = m_active & (den > zero);
#else
    mic_m m_valid = m_active;
#endif

    const mic_f u = dot(cross(v2+v0,e0),ray_dir) * rcp_den; 
    m_valid       = ge( m_valid, u, zero);

    const mic_f v       = dot(cross(v0+v1,e1),ray_dir) * rcp_den; 
    m_valid       = ge( m_valid, v, zero);

    const mic_f w       = dot(cross(v1+v2,e2),ray_dir) * rcp_den;  
    m_valid       = ge( m_valid, w, zero);

    if (unlikely(none(m_valid))) return false;
    
    const mic_m m_den = ne(m_valid,den,zero);
    const mic_f t = dot(v0,Ng) * rcp_den;
    mic_f max_dist_xyz = mic_f(ray.tfar);
    mic_m m_final      = lt(lt(m_den,mic_f(ray.tnear),t),t,max_dist_xyz);

    //////////////////////////////////////////////////////////////////////////////////////////////////

    if (unlikely(any(m_final))) return true;
    return false;
  };

  
  static __forceinline void intersect1_quad16(const mic_f &dir_xyz,
					      const mic_f &org_xyz,
					      Ray& ray,
					      const mic3f &vtx,
					      const mic_f &u,
					      const mic_f &v,
					      const unsigned int grid_res,
					      const mic_m &m_active,
					      const unsigned int subdiv_patch_index)
  {
    const unsigned int offset_v0 = 0;
    const unsigned int offset_v1 = 1;
    const unsigned int offset_v2 = grid_res+1;
    const unsigned int offset_v3 = grid_res+0;


    const mic3f ray_org(swAAAA(org_xyz),swBBBB(org_xyz),swCCCC(org_xyz));
    
    mic3f vtx_org = vtx - ray_org;

    const mic3f &v0 = vtx_org;
    const mic3f  v1( uload16f_low(&vtx_org.x[offset_v1]), uload16f_low(&vtx_org.y[offset_v1]), uload16f_low(&vtx_org.z[offset_v1]));
    const mic3f  v2( uload16f_low(&vtx_org.x[offset_v2]), uload16f_low(&vtx_org.y[offset_v2]), uload16f_low(&vtx_org.z[offset_v2]));
    const mic3f  v3( uload16f_low(&vtx_org.x[offset_v3]), uload16f_low(&vtx_org.y[offset_v3]), uload16f_low(&vtx_org.z[offset_v3]));

    intersect1_tri16_precise(dir_xyz,org_xyz,ray,v0,v1,v3,u,v,offset_v0,offset_v1,offset_v3,m_active,subdiv_patch_index);
    intersect1_tri16_precise(dir_xyz,org_xyz,ray,v3,v1,v2,u,v,offset_v3,offset_v1,offset_v2,m_active,subdiv_patch_index);

  }

  static __forceinline bool occluded1_quad16(const mic_f &dir_xyz,
					     const mic_f &org_xyz,
					     Ray& ray,
					     const mic3f &vtx,
					     const mic_f &u,
					     const mic_f &v,
					     const unsigned int grid_res,
					     const mic_m &m_active,
					     const unsigned int subdiv_patch_index)
  {
    const unsigned int offset_v0 = 0;
    const unsigned int offset_v1 = 1;
    const unsigned int offset_v2 = grid_res+1;
    const unsigned int offset_v3 = grid_res+0;

    const mic3f ray_org(swAAAA(org_xyz),swBBBB(org_xyz),swCCCC(org_xyz));
    
    mic3f vtx_org = vtx - ray_org;

    const mic3f &v0 = vtx_org;
    const mic3f  v1( uload16f_low(&vtx_org.x[offset_v1]), uload16f_low(&vtx_org.y[offset_v1]), uload16f_low(&vtx_org.z[offset_v1]));
    const mic3f  v2( uload16f_low(&vtx_org.x[offset_v2]), uload16f_low(&vtx_org.y[offset_v2]), uload16f_low(&vtx_org.z[offset_v2]));
    const mic3f  v3( uload16f_low(&vtx_org.x[offset_v3]), uload16f_low(&vtx_org.y[offset_v3]), uload16f_low(&vtx_org.z[offset_v3]));

    if (occluded1_tri16_precise(dir_xyz,org_xyz,ray,v0,v1,v3,u,v,offset_v0,offset_v1,offset_v3,m_active,subdiv_patch_index)) return true;
    if (occluded1_tri16_precise(dir_xyz,org_xyz,ray,v3,v1,v2,u,v,offset_v3,offset_v1,offset_v2,m_active,subdiv_patch_index)) return true;
    return false;
  }


};

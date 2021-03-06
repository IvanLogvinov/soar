/*
    This file is part of Sorts, an interface between Soar and ORTS.
    (c) 2006 James Irizarry, Sam Wintermute, and Joseph Xu

    Sorts is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    Sorts is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with Sorts; if not, write to the Free Software
    Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA    
*/
#include "include/PerceptualGroupManager.h"
#include "include/general.h"
#include <iostream>

#include "PerceptualGroup.h"
#include "Sorts.h"
#include "SoarInterface.h"

#define CLASS_TOKEN "GRPMAN"
#define DEBUG_OUTPUT true 
#include "OutputDefinitionsUnique.h"

using namespace std;

/*
  PerceptualGroupManager.cpp
  SORTS project
  Sam Wintermute, 2006

*/


PerceptualGroupManager::PerceptualGroupManager() {
  // sorts ptr is NOT valid here, use the intialize() function! 
    
  // this default should be reflected in the agent's assumptions
  // (1024 = 32^2)
  visionParams.groupingRadius = 0;
#ifdef GAME_ONE
  visionParams.groupingRadius = 999999;
#endif
  
  // the number of objects near the focus point to add
  // agent can change this, if it wishes to cheat
  visionParams.numObjects = 200;//100;

  visionParams.ownerGrouping = false;

  counter = 0;
}

void PerceptualGroupManager::initialize() {
  // called right after sorts ptr set up
  visionParams.focusX = (int) (Sorts::OrtsIO->getMapXDim() / 2.0);
  visionParams.focusY = (int) (Sorts::OrtsIO->getMapYDim() / 2.0);

  visionParams.centerX = visionParams.focusX;
  visionParams.centerY = visionParams.focusY;

  visionParams.viewWidth = 2*visionParams.focusX;
  
  msg << "init: " << visionParams.focusX << " " << visionParams.focusY 
       << " " << visionParams.viewWidth << endl;

  Sorts::featureMapManager->changeViewWindow(visionParams.focusX, 
                                             visionParams.focusY, 
                                             visionParams.viewWidth);

  Sorts::SoarIO->initVisionState(visionParams);
}
void PerceptualGroupManager::updateGroups() {
  prepareForReGroup();
    // prune empty groups (if units died)
    // prepare list of group categories that need to be reGrouped
    // recalculate the center member for groups that changed

  if (visionParams.groupingRadius == 0) {
    staleGroupCategories.clear();
  }
  else {
    reGroup();
  }
    
  generateGroupData();
    // prune groups emptied during reGrouping
    // aggregate data about the groups
  
  adjustAttention(false);
    // determine which groups are attended to,
    // and send them to Soar

#ifdef GAME_ONE // should be "NO_SOAR"
#ifdef GAME_FOUR
  
  list<PerceptualGroup*> enemies;
  for (set<PerceptualGroup*>::iterator groupIter = perceptualGroups.begin(); 
       groupIter != perceptualGroups.end(); 
       groupIter++) {
    if (((*groupIter)->getName() == "marine" ||
	 (*groupIter)->getName() == "controlCenter")
          and not (*groupIter)->isFriendly()) {
      enemies.push_back(*groupIter);
    }
  }
  for (set<PerceptualGroup*>::iterator groupIter = perceptualGroups.begin(); 
       groupIter != perceptualGroups.end(); 
       groupIter++) {
    if ((*groupIter)->getName() == "marine" and (*groupIter)->isFriendly() and
        (not (*groupIter)->getHasCommand() || Sorts::frame % 50 == 0)) {
      list<int> params;
      (*groupIter)->assignAction(OA_ATTACK, params, enemies);
    }
  }
#else
  for (set<PerceptualGroup*>::iterator groupIter = perceptualGroups.begin(); 
       groupIter != perceptualGroups.end(); 
       groupIter++) {
      if ((*groupIter)->isFriendlyWorker() and not (*groupIter)->getHasCommand()) {
        list<int> params;
        list<PerceptualGroup*> groups;
        (*groupIter)->assignAction(OA_MINE, params, groups);
      }
    }
#endif 
#endif
  return;
}

bool PerceptualGroupManager::assignActions() {
#ifndef GAME_ONE
  // through the Sorts::getSoarInterface(), look for any new actions, and assign them to groups
  // actions have a list of params and a list of groups,
  // the first group (must exist) is the group the action will be applied to
    
  list <ObjectAction> newActions;
  
  Sorts::SoarIO->getNewObjectActions(newActions);
  list <ObjectAction>::iterator actionIter = newActions.begin();

  msg << "assigning " << newActions.size() << " new actions.\n";
 
  list <PerceptualGroup*>::iterator groupIter;
  bool success = true;
  list<PerceptualGroup*> targetGroups;
  PerceptualGroup* sourceGroup;
  
  while (actionIter != newActions.end()){
    if ((*actionIter).type == OA_ATTACK){
      list<PerceptualGroup*> enemies;
      for (set<PerceptualGroup*>::iterator groupIter=perceptualGroups.begin();
	   groupIter != perceptualGroups.end(); 
           groupIter++) {
        if (((*groupIter)->getName() == "marine" || (*groupIter)->getName() == "tank" ||
	         (*groupIter)->getName() == "worker" || (*groupIter)->getName() == "controlCenter")
	         and not (*groupIter)->isFriendly()) {
          enemies.push_back(*groupIter);
        }
      }

      success &= (*(*actionIter).groups.begin())->assignAction(
		 (*actionIter).type, (*actionIter).params, enemies);
      actionIter++;
      cout << "erm" << endl;
    } else
    if ((*actionIter).type != OA_NO_SUCH_ACTION) {
      targetGroups.clear();
      list<PerceptualGroup*>& groups = (*actionIter).groups;
      groupIter = groups.begin();
      
      ASSERT(groupIter != groups.end());
      // the first group is the group the action is applied to, it must exist
      
      sourceGroup = *groupIter;
      groupIter++;
      
      while (groupIter != groups.end()) {
        targetGroups.push_back(*groupIter);
        groupIter++;
      }
      
      success &= sourceGroup->assignAction(
              (*actionIter).type, (*actionIter).params, targetGroups);
      
      actionIter++;
    }
  }
  dbg << "done assigning.\n";
#endif
  return true;
}

void PerceptualGroupManager::updateQueryDistances() {
  // called from MapQuery, need to re-generate all gp. data so new
  // distToQuery is picked up

  for (set<PerceptualGroup*>::iterator groupIter = perceptualGroups.begin(); 
       groupIter != perceptualGroups.end(); 
       groupIter++) {
    // inSoar status can't change in this situation
    if ((*groupIter)->getInSoar() == true) {
      (*groupIter)->generateData();  
      Sorts::SoarIO->refreshGroup(*groupIter);
      Sorts::canvas.unregisterGroup(*groupIter);
      Sorts::canvas.registerGroup(*groupIter);
      
    }
  }
}
  

void PerceptualGroupManager::processVisionCommands() {
#ifndef GAME_ONE
  // called when Soar changes the view window, wants to attend to an item
  // in a feature map, or changes a grouping parameter.

  // view window change:
  // call Sorts::getFeatureMapManager()->changeViewWindow()
  // updateFeatureMaps(true)

  // attention shift (w/o view window shift):
  // call adjustAttention() to select the new groups
  // call updateFeatureMaps(false) to inhibit any newly-attended to groups

  // attention shift w/ view window shift:
  // Sorts::getFeatureMapManager()->changeViewWindow()
  // adjustAttention()
  // updateFeatureMaps(true)

  // grouping change:
  // this is the same as updateVision, except we don't need prepareForReGroup,
  // since none of the objects in the world actually changed:
  // reGroup()
  // generateGroupData()
  // adjustAttention()
  // updateFeatureMaps(false)
  
  list<AttentionAction> actions;
  Sorts::SoarIO->getNewAttentionActions(actions);

  int radius;
  PerceptualGroup* centerGroup;
  list<int>::iterator it;

  for (list<AttentionAction>::iterator i = actions.begin();
                                       i != actions.end();
                                       i++) {
    switch (i->type) {
      case AA_LOOK_LOCATION:
        ASSERT(i->params.size() == 2); // x,y
        it = i->params.begin();
        visionParams.focusX = *it;
        it++;
        visionParams.focusY = *it;
        
        // recalc all center distances and rebuild the order of the groups
        remakeGroupSet();
        adjustAttention(false); 

        break;
      case AA_LOOK_FEATURE:
        // attention shift (w/o view window shift)

        // first (and only) param is the sector number
        ASSERT(i->params.size() == 1);
        centerGroup = Sorts::featureMapManager->getGroup(i->fmName, 
                                                        *(i->params.begin()));
        if (centerGroup == NULL) {
          msg << "ERROR: sector " << *(i->params.begin()) << " of map " <<
            i->fmName << " is empty! Ignoring command\n";
          return;
        }
        
        // set focus point the center of the group
        centerGroup->getCenterLoc(visionParams.focusX, visionParams.focusY);

        // recalc all center distances and rebuild the order of the groups
        remakeGroupSet();
        adjustAttention(false); 
        break;
      case AA_RESIZE:
        ASSERT(i->params.size() == 1);
        visionParams.viewWidth = *(i->params.begin());
        Sorts::featureMapManager->changeViewWindow(visionParams.centerX, 
                                                   visionParams.centerY, 
                                                   visionParams.viewWidth);
        adjustAttention(true);   // re-update all the feature maps-
                                 // changeViewWindow clears them out
        
        Sorts::SoarIO->updateVisionState(visionParams); 
        break;
      case AA_MOVE_LOCATION:
        ASSERT(i->params.size() == 2);
        it = i->params.begin();
        visionParams.focusX = *it;
        ++it;
        visionParams.focusY = *it;
        visionParams.centerX = visionParams.focusX;
        visionParams.centerY = visionParams.focusY;
        Sorts::featureMapManager->changeViewWindow(visionParams.centerX, 
                                                   visionParams.centerY, 
                                                   visionParams.viewWidth);

        // recalc all center distances and rebuild the order of the groups
        remakeGroupSet();
        Sorts::featureMapManager->changeViewWindow(visionParams.centerX, 
                                                   visionParams.centerY, 
                                                   visionParams.viewWidth);
        adjustAttention(true);   // re-update all the feature maps-
                                 // changeViewWindow clears them out
        Sorts::SoarIO->updateVisionState(visionParams); 
        break;
      case AA_MOVE_FEATURE:
        ASSERT(i->params.size() == 1);
        centerGroup = Sorts::featureMapManager->getGroup(i->fmName, 
                                                        *(i->params.begin()));
        if (centerGroup == NULL) {
          msg << "ERROR: sector " << *(i->params.begin()) << " of map " <<
            i->fmName << " is empty! Ignoring command\n";
          return;
        }
        
        centerGroup->getCenterLoc(visionParams.focusX, visionParams.focusY);
        visionParams.centerX = visionParams.focusX;
        visionParams.centerY = visionParams.focusY;
        Sorts::featureMapManager->changeViewWindow(visionParams.centerX, 
                                                   visionParams.centerY, 
                                                   visionParams.viewWidth);

        // recalc all center distances and rebuild the order of the groups
        remakeGroupSet();
        Sorts::featureMapManager->changeViewWindow(visionParams.centerX, 
                                                   visionParams.centerY, 
                                                   visionParams.viewWidth);
        adjustAttention(true);   // re-update all the feature maps-
                                 // changeViewWindow clears them out
        Sorts::SoarIO->updateVisionState(visionParams); 
        break;
      case AA_GROUPING_RADIUS:
      // grouping change:
      // this is the same as updateVision, except we don't need prepareForReGroup,
      // since none of the objects in the world actually changed:
        ASSERT(i->params.size() == 1);
        radius = *(i->params.begin());
        radius *= radius;
        if (radius != visionParams.groupingRadius) {
          visionParams.groupingRadius = radius;
          setAllCategoriesStale();
          reGroup();
          generateGroupData();
          adjustAttention(false);
          Sorts::SoarIO->updateVisionState(visionParams); 
        }
        break;
      case AA_NUM_OBJECTS:
        ASSERT(i->params.size() == 1);
        visionParams.numObjects = *(i->params.begin());
        adjustAttention(false);
        Sorts::SoarIO->updateVisionState(visionParams); 
        break;
      case AA_OWNER_GROUPING_ON:
        // handle similar to grouping radius change-
        // rebuild all the groups
        if (visionParams.ownerGrouping) {
          msg << "WARNING: turning on ownerGrouping when it is already on.\n";
          break;
        }
        visionParams.ownerGrouping = true; 
        // this essentially changes the definition of category
        
        setAllCategoriesStale();
        reGroup();
        generateGroupData();
        adjustAttention(false);
        Sorts::SoarIO->updateVisionState(visionParams); 
        break;
      case AA_OWNER_GROUPING_OFF:
        if (not visionParams.ownerGrouping) {
          msg << "WARNING: turning off ownerGrouping when it is already off.\n";
          break;
        }
        visionParams.ownerGrouping = false; 
        
        setAllCategoriesStale();
        reGroup();
        generateGroupData();
        adjustAttention(false);
        Sorts::SoarIO->updateVisionState(visionParams); 
        break;
      case AA_NO_SUCH_ACTION:
        break;
      default:
        break;
    }
  }

#endif
}

void PerceptualGroupManager::prepareForReGroup() {
  // iterate through all the groups, if they are stale,
  // recalculate the center member
 
  // prune empty groups

  // add the group type of stale groups to
  // the staleGroupCategories sets, so reGroupX will run on them
 
  set<PerceptualGroup*>::iterator groupIter;

  list<set<PerceptualGroup*>::iterator> toErase;
  
  for (groupIter = perceptualGroups.begin();
       groupIter != perceptualGroups.end(); 
       groupIter++) {
    if ((*groupIter)->getHasStaleMembers()) {
      if ((*groupIter)->isEmpty()) {
        toErase.push_back(groupIter);
        // can't delete from sets like this- the iterator gets screwed up
        // keep a list, delete everything at once
      }
      else {
        (*groupIter)->updateCenterMember();
        staleGroupCategories.insert(
            (*groupIter)->getCategory(visionParams.ownerGrouping));
      }
    }
  }
  for (list<set<PerceptualGroup*>::iterator>::iterator it= toErase.begin();
      it != toErase.end();
      it++) {
    removeGroup(**it);
    perceptualGroups.erase(*it);
  }

  //msg << "end ref" << endl;
  return;
}


void PerceptualGroupManager::reGroup() {
  // iterate through staleGroupCategories set
  //  find all the groups of each type
  //  add the members to a big list, centers first
  //  keep a struct for each object:
  //    ptr to the obj, flag for if it has been assigned a group, ptr to group
  //  go through each obj1 in the list
  //    if not a group-center and not flagged, rm from old group and make a new group
  //    check each object (obj2) below in list:
  //      if objs are close and obj2 not flagged, flag obj2 and bring to same group
  //      if objs are close and obj2 flagged, check oldGroup flag
  //        merge groups, preferring to keep groups w/ oldgroup flag
  //        if neither is set, choice is arbitrary
  //        if both are set, prefer the larger group

  set<pair<string, int> >::iterator catIter = staleGroupCategories.begin();
  perceptualGroupingStruct objectData;
  list<SoarGameObject*> groupMembers;
  set<PerceptualGroup*>::iterator groupIter;
  list<SoarGameObject*>::iterator objectIter;
  
  list<perceptualGroupingStruct> groupingList;
  list<perceptualGroupingStruct> centerGroupingList;

  // save all the to-merge pairs in a list
  // do all the merges at the end
  // this prevents invalid groups in the list (groups are deleted after a merge)
  list<pair<PerceptualGroup*, PerceptualGroup*> > toMergeList;
  
  SoarGameObject* centerObject;

  PerceptualGroup* newGroup;
  int size;

  while (catIter != staleGroupCategories.end()) {
//    msg << "doing type " << catIter->first << endl;
    groupingList.clear();
    centerGroupingList.clear();
    
    for (groupIter = perceptualGroups.begin(); 
         groupIter != perceptualGroups.end(); 
         groupIter++) {
      if (not (*groupIter)->getSticky() and
         ((*groupIter)->getCategory(visionParams.ownerGrouping) == *catIter)) {
  //      msg << "group " << (int) (*groupIter) << endl;
        // group is of the type we are re-grouping
       
        if ((*groupIter)->isOld()) {
          // oldGroup means the group has been around for at least one cycle
          // centers of old groups have priority for retaining their
          // group ID
          objectData.group = *groupIter;
          objectData.assigned = false;
          centerObject = (*groupIter)->getCenterMember();
      
          // centers are stored in a separate list
          objectData.object = centerObject;
          objectData.x = *centerObject->getGob()->sod.x;
          objectData.y = *centerObject->getGob()->sod.y;
          objectData.oldGroup = true;
          
          centerGroupingList.push_back(objectData);
          objectData.oldGroup = false;
          (*groupIter)->getMembers(groupMembers);
    //      msg << "group has " << groupMembers.size() << " members.\n";
          objectIter = groupMembers.begin();
          while (objectIter != groupMembers.end()) {
            if ((*objectIter) != centerObject){
              // don't add the center object to this list
              objectData.object = *objectIter;
              
              objectData.x = *(*objectIter)->getGob()->sod.x;
              objectData.y = *(*objectIter)->getGob()->sod.y;
              groupingList.push_back(objectData);
            }
            objectIter++;
          }
        }
        else {
          // not an old group- don't care about center distinction
          objectData.group = *groupIter;
          objectData.assigned = false;
          objectData.oldGroup = false;
          (*groupIter)->getMembers(groupMembers);
          objectIter = groupMembers.begin();
          while (objectIter != groupMembers.end()) {
            objectData.object = *objectIter;
            objectData.x = *(*objectIter)->getGob()->sod.x;
            objectData.y = *(*objectIter)->getGob()->sod.y;
            groupingList.push_back(objectData);
            objectIter++;
          }
        }
      }
      // else it was a group of a different type
    }
    // the lists are now built, centers in a separate list we will
    // treat as being "before" the other list
    
    // now follow the grouping procedure as outlined above
    bool obj1IsACenter = true;
    list<perceptualGroupingStruct>::iterator obj1StructIter, obj2StructIter;
    perceptualGroupingStruct obj1Struct;
    
    obj1StructIter = centerGroupingList.begin();
    if (obj1StructIter == centerGroupingList.end()) {
      // this really should not happen
      obj1StructIter = groupingList.begin();
      obj1IsACenter = false;
    }
    while (obj1StructIter != groupingList.end()) {
      obj1Struct = *obj1StructIter;
      
      if (not obj1IsACenter and not obj1Struct.assigned) {
        // make a new group for this object- no existing 
        // group has claimed it yet
        obj1Struct.group->removeUnit(obj1Struct.object);
        newGroup = new PerceptualGroup(obj1Struct.object);
        newGroup->calcDistToFocus(visionParams.focusX, visionParams.focusY);
        size = perceptualGroups.size();
        perceptualGroups.insert(newGroup);
        ASSERT(perceptualGroups.size() == size + 1);
        if (perceptualGroups.size() != size + 1) {
          // if dbg is off
          msg << "ERROR: bad insertion!\n";
        }
        obj1Struct.group = newGroup;
        obj1Struct.assigned = true;
   //     msg << "ILNK making new group " << (int) obj1Struct.group << endl; 
      }
     
      // iterate through all lower objects to see if they should join the group
      obj2StructIter = obj1StructIter;
      obj2StructIter++;
      if (obj2StructIter == centerGroupingList.end()) {
        obj2StructIter = groupingList.begin();
      }
      while (obj2StructIter != groupingList.end()) {
        if (squaredDistance(obj1Struct.x, obj1Struct.y, 
                            (*obj2StructIter).x, (*obj2StructIter).y)
            <= visionParams.groupingRadius) {
          if ((*obj2StructIter).assigned) {
            // obj2 already has been grouped- groups should merge
            pair<PerceptualGroup*, PerceptualGroup*> groups;

            if (obj1Struct.oldGroup and not (*obj2StructIter).oldGroup) {
              // obj1's group isn't new, and 2's is, keep 1's group
              groups.second = obj1Struct.group;
              groups.first = (*obj2StructIter).group;
            }
            else if (not obj1Struct.oldGroup and (*obj2StructIter).oldGroup) {
              // vice versa
              groups.first = obj1Struct.group;
              groups.second = (*obj2StructIter).group;
            }
            else if (not obj1Struct.oldGroup and not (*obj2StructIter).oldGroup) {
              // arbitrary
              groups.first = obj1Struct.group;
              groups.second = (*obj2StructIter).group;
            }
            else {
              // both old- keep the bigger
              if (obj1Struct.group->getSize() 
                  > (*obj2StructIter).group->getSize()) {
                groups.second = obj1Struct.group;
                groups.first = (*obj2StructIter).group;
              }
              else {
                groups.first = obj1Struct.group;
                groups.second = (*obj2StructIter).group;
              }
            }
            toMergeList.push_back(groups);
    //        msg << "ILNK will merge " << (int) groups.first << " -> " << (int) groups.second << endl;
          }
          else {
            // obj2 has not been assigned. Assign it to obj1's group.
      //      msg << "ILNK obj from group " << (int) (*obj2StructIter).group <<
      //              " joining " << (int) obj1Struct.group << endl;
            (*obj2StructIter).assigned = true;
            (*obj2StructIter).group->removeUnit((*obj2StructIter).object);
            (*obj2StructIter).group = obj1Struct.group;
            (*obj2StructIter).group->addUnit((*obj2StructIter).object);
            (*obj2StructIter).oldGroup = obj1Struct.oldGroup;
            
          }
     //     msg << "grouped!" << endl;
        }
        else {
     //     msg << "not grouped!" << endl;
        }
        obj2StructIter++; 
        if (obj2StructIter == centerGroupingList.end()) {
          obj2StructIter = groupingList.begin();
        }
      }
      // jump the iterator between the two lists
      obj1StructIter++;
      if (obj1StructIter == centerGroupingList.end()) {
        obj1StructIter = groupingList.begin();
        obj1IsACenter = false;
      }
    }
    catIter++;
  } // end iterating through all the types that need re-grouping
  
  // do merges- always merge the first group to the second
  
  list<pair<PerceptualGroup*, PerceptualGroup*> >::iterator toMergeIter;
  list<pair<PerceptualGroup*, PerceptualGroup*> >::iterator toMergeIter2;

  // if two groups merge, we need to ensure that the subsumed group
  // does not have any outstanding merges
  toMergeIter = toMergeList.begin();
  while (toMergeIter != toMergeList.end()) {
    //msg << "groups " << (int)  (*toMergeIter).first << " and " << (int) (*toMergeIter).second << " will merge\n";
    if ((*toMergeIter).first == (*toMergeIter).second) {
      // do nothing- the groups were already merged
    }
    else {
      // merge first into second
      
      toMergeIter2 = toMergeIter;
      toMergeIter2++;
      while (toMergeIter2 != toMergeList.end()) {
        // replace all occurrences of the squashed group with the
        // new combined group
        if ((*toMergeIter2).first == (*toMergeIter).first) {
          (*toMergeIter2).first = (*toMergeIter).second;
        }
        if ((*toMergeIter2).second == (*toMergeIter).first) {
          (*toMergeIter2).second = (*toMergeIter).second;
        }
        toMergeIter2++;
      }

      (*toMergeIter).first->mergeTo((*toMergeIter).second);
    }
    
    toMergeIter++;
  }
 
  staleGroupCategories.clear();
//  msg << "ILNK regroup done" << endl;
  return;
}


void PerceptualGroupManager::removeGroup(PerceptualGroup* group) {
  if (group->getInSoar()) {
    Sorts::SoarIO->removeGroup(group);
    Sorts::canvas.unregisterGroup(group);
  }
  Sorts::featureMapManager->removeGroup(group);
  delete group;
}

void PerceptualGroupManager::generateGroupData() {
  // iterate through all the groups, if they are stale,
  // refresh them (re-calc stats)
 
  set<PerceptualGroup*>::iterator groupIter;
  list<set<PerceptualGroup*>::iterator> toErase;
  list<set<PerceptualGroup*>::iterator> toReinsert;

  for (groupIter = perceptualGroups.begin(); 
       groupIter != perceptualGroups.end(); 
       groupIter++) {
    if ((*groupIter)->getHasStaleMembers()) {
      if ((*groupIter)->isEmpty()) {
        toErase.push_back(groupIter);
        removeGroup(*groupIter);
      }
      else {
        (*groupIter)->generateData();
        (*groupIter)->calcDistToFocus(visionParams.focusX, visionParams.focusY);
        // groups that have stale members need to be removed and reinserted
        // this is because the set is maintained in order of distance from
        // the focus center, and a stale-membered group could have had this 
        // distance changed
        toReinsert.push_back(groupIter);
      }
    }
  }
  
  for (list<set<PerceptualGroup*>::iterator>::iterator it= toErase.begin();
      it != toErase.end();
      it++) {
    perceptualGroups.erase(*it);
  }
  
  PerceptualGroup* grp;
  for (list<set<PerceptualGroup*>::iterator>::iterator it= toReinsert.begin();
      it != toReinsert.end();
      it++) {
    grp = **it;
    perceptualGroups.erase(*it);
    int size = perceptualGroups.size();
    perceptualGroups.insert(grp);
    ASSERT(perceptualGroups.size() == size+1);
    if (perceptualGroups.size() != size + 1) {
      // if dbg is off
      msg << "ERROR: bad insertion!\n";
    }
  }

  return;
}


void PerceptualGroupManager::makeNewGroup(SoarGameObject* object) {
  PerceptualGroup* newGroup;
  newGroup = new PerceptualGroup(object);
  newGroup->calcDistToFocus(visionParams.focusX, visionParams.focusY);
  int size = perceptualGroups.size();
  perceptualGroups.insert(newGroup);
  ASSERT(perceptualGroups.size() == size+1);
  if (perceptualGroups.size() != size + 1) {
    // if dbg is off
    msg << "ERROR: bad insertion!\n";
  }

  return;
}

void PerceptualGroupManager::adjustAttention(bool rebuildFeatureMaps) {
  // iterate through all staleroperties groups, if in attn. range,
  // send params to Soar
  
  set<PerceptualGroup*>::iterator groupIter;
  int i=0;
  int totalObjs = 0;
  for (groupIter = perceptualGroups.begin(); 
       groupIter != perceptualGroups.end(); 
       groupIter++) {
    totalObjs += (*groupIter)->getSize();
    if (i < visionParams.numObjects) {
      if (not (*groupIter)->getInSoar()) {
        Sorts::SoarIO->addGroup(*groupIter); 
        
        Sorts::SoarIO->refreshGroup(*groupIter);
        (*groupIter)->setInSoar(true);
        
        // recently added / removed from Soar is a stale property, 
        // as far as feature maps are concerned- the inhibition of
        // the group must change
        (*groupIter)->setHasStaleProperties(true);
        Sorts::canvas.registerGroup(*groupIter);
      }
      else if ((*groupIter)->getHasStaleProperties()) {
        Sorts::SoarIO->refreshGroup(*groupIter);
        Sorts::canvas.unregisterGroup(*groupIter);
        Sorts::canvas.registerGroup(*groupIter);
      }
      else {
        dbg << "ILNK group " << (*groupIter) 
            << " hasn't changed, but present\n";
        pair<string,int> cat = (*groupIter)->getCategory(false);
        dbg << "ILNK type: " << cat.first << "\n";
        dbg << "ILNK owner: " << cat.second << "\n";
        dbg << "ILNK members: " << (*groupIter)->getSize() << "\n";
      }
    }
    else { 
      if ((*groupIter)->getInSoar() == true) {
          // HACK: always attend to groups with commands
          // (non-idle statuses)
        if (not (*groupIter)->getHasCommand()) {
          (*groupIter)->setInSoar(false);
          Sorts::SoarIO->removeGroup(*groupIter);
          Sorts::canvas.unregisterGroup(*groupIter);
          (*groupIter)->setHasStaleProperties(true);
        }
        else if ((*groupIter)->getHasStaleProperties()) {
          dbg << "refresh due to command (out of normal range)\n";
          Sorts::SoarIO->refreshGroup(*groupIter);
          Sorts::canvas.unregisterGroup(*groupIter);
          Sorts::canvas.registerGroup(*groupIter);
        }
      }
      else {
        dbg << "ILNK group " << (*groupIter) 
            << " is out of attention.\n";
      }
    }
    i++;
  }
  dbg << "TOTALO: " << totalObjs << "\n";;
  if (totalObjs != (110- Sorts::OrtsIO->getDeadCount())) {
    //dbg << "ERROR: if this is game 2, we expect " << 110- Sorts::OrtsIO->getDeadCount() << " objects.\n";
  }
  if (rebuildFeatureMaps) {
    // do this after view window changes-
    // the change results in empty feature maps
    // re-add all groups (via refresh) to the feature maps

    // FeatureMapManager can't do this because it doesn't know what all the 
    // groups are.
    
    list <FeatureMap*> emptyList;
    
    for (groupIter = perceptualGroups.begin(); 
         groupIter != perceptualGroups.end(); 
         groupIter++) {
      // sector == -1 means that the group is not in any fmaps
      (*groupIter)->setFMSector(-1);
      Sorts::featureMapManager->refreshGroup(*groupIter);
      (*groupIter)->setHasStaleProperties(false);
    }
  }
  else {
    // just refresh groups that changed in the feature maps
    for (groupIter = perceptualGroups.begin(); 
         groupIter != perceptualGroups.end(); 
         groupIter++) {
      if ((*groupIter)->getHasStaleProperties()) {
        Sorts::featureMapManager->refreshGroup(*groupIter);
        (*groupIter)->setHasStaleProperties(false);
      }
    }
  }

  Sorts::featureMapManager->updateSoar();
  return;
}

PerceptualGroupManager::~PerceptualGroupManager() {
  set<PerceptualGroup*>::iterator groupIter;
  for (groupIter = perceptualGroups.begin(); 
       groupIter != perceptualGroups.end(); 
       groupIter++) {
    delete (*groupIter);
  }
}



void PerceptualGroupManager::setAllCategoriesStale() {
  // add all categories to the stale list
  // (used to force all groups to refresh after grouping params change)
  staleGroupCategories.clear();
  
  set<PerceptualGroup*>::iterator groupIter;
  // jump iterator between lists

  for (groupIter = perceptualGroups.begin(); 
      groupIter != perceptualGroups.end(); 
      groupIter++) {
    staleGroupCategories.insert(
        (*groupIter)->getCategory(visionParams.ownerGrouping));
  }
}

void PerceptualGroupManager::remakeGroupSet() {
  // if the focus point changes, all groups need to be reinserted in the
  // group set, since it is maintained in order of distance from the center

  set<PerceptualGroup*>::iterator groupIter;
 
  set<PerceptualGroup*, ltGroupPtr> newSet;
  
  for (groupIter = perceptualGroups.begin(); 
      groupIter != perceptualGroups.end(); 
      groupIter++) {
    (*groupIter)->calcDistToFocus(visionParams.focusX, visionParams.focusY);
    newSet.insert(*groupIter);
  }

  perceptualGroups = newSet;
}

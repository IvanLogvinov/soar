#include <sys/time.h>

#include "AttackManager.h"
#include "general.h"
#include "AttackManagerRegistry.h"
#include "Circle.h"
#include "Sorts.h"

#include "ScriptObj.H"

#define msg cout << "AttackManager.cpp: "

#define MAX_INDIVIDUAL_AHEAD 0.5

#ifdef USE_CANVAS
#define USE_CANVAS_ATTACK_MANAGER
#endif


void AttackManager::attackArcPos
( GameObj* atk, 
  GameObj* tgt, 
  list<Vec2d>& positions) 
{
  int range;
  int atkRadius = *atk->sod.radius;
  int tgtRadius; 
  if (*tgt->sod.shape == SHAPE_RECTANGLE) {
    int tgtHeight = *tgt->sod.y2 - *tgt->sod.y1;
    int tgtWidth  = *tgt->sod.x2 - *tgt->sod.x1;
    assert(tgtHeight > 0 && tgtWidth > 0);
    if (tgtHeight < tgtWidth) {
      tgtRadius = tgtHeight / 2;
    }
    else {
      tgtRadius = tgtWidth / 2;
    }
  }
  else {
    tgtRadius = *tgt->sod.radius;
  }
  Vec2d aPos(*atk->sod.x, *atk->sod.y);
  Vec2d tPos(*tgt->sod.x, *tgt->sod.y);
  if (*tgt->sod.zcat == GameObj::ON_LAND) {
    range = atk->component("weapon")->get_int("max_ground_range") 
      + atkRadius + tgtRadius;
  }
  else {
    range = atk->component("weapon")->get_int("max_air_range") 
      + atkRadius + tgtRadius;
  }

  range = range - 3; // for safety

  Vec2d closestPos = tPos - Vec2d(tPos - aPos, range);
  list<Vec2d> atkPos;
  positionsOnCircle(tPos, closestPos, *atk->sod.radius * 2, atkPos);
  
  for(list<Vec2d>::iterator
      i  = atkPos.begin();
      i != atkPos.end();
      i++) 
  {
    list<GameObj*> collisions;
    Vec2d intPos = i->roundToward(tPos);
    if (0 <= intPos(0) && intPos(0) <= Sorts::OrtsIO->getMapXDim() && 
        0 <= intPos(1) && intPos(1) <= Sorts::OrtsIO->getMapYDim()) 
    {
      bool slotTaken = false;
      for(list<AttackFSM*>::iterator
          j =  team.begin();
          j != team.end();
          j++) 
      {
        if ((*j)->isMoving()) {
          double d = (intPos - (*j)->getDestination()).magSq();
          double r = *(*j)->getGob()->sod.radius + atkRadius;
          if (d < r * r) {
            slotTaken = true;
            break;
          }
        }
      }
      if (!slotTaken) {
        if (!Sorts::spatialDB->hasObjectCollision
              (intPos(0), intPos(1), atkRadius)
     //       and
     //       !Sorts::spatialDB->hasTerrainCollision
     //         (intPos(0), intPos(1), atkRadius)
           )
        {
          positions.push_back(intPos);
        }
      }
    }
  //positions.push_back(intPos);
  }
}

AttackManager::AttackManager(const set<SoarGameObject*>& _targets)
: targetSet(_targets)
{
  for(set<SoarGameObject*>::iterator
      i  = targetSet.begin();
      i != targetSet.end();
      ++i)
  {
    targets.insert(pair<SoarGameObject*, AttackTargetInfo>(*i, AttackTargetInfo(*i)));
  }
  reprioritize();

#ifdef USE_CANVAS_ATTACK_MANAGER
  Uint8 r = (Uint8) (((int) this) % 156) + 100;
  for(set<SoarGameObject*>::iterator
      i  = targets.begin();
      i != targets.end();
      ++i)
  {
    if (!Sorts::canvas.gobRegistered((*i)->getGob())) {
      Sorts::canvas.registerGob((*i)->getGob());
      Sorts::canvas.setColor((*i)->getGob(), r, 0, 0);
    }
  }
#endif
}

AttackManager::~AttackManager() {
  int status;
  if (targets.size() == 0) {
    status = 1;
  }
  else {
    status = 0;
  }

  for(list<AttackFSM*>::iterator
      i =  team.begin();
      i != team.end();
      ++i)
  {
#ifdef USE_CANVAS_ATTACK_MANAGER
    Sorts::canvas.unregisterGob((*i)->getGob());
#endif
    (*i)->disown(status);
  }
}

void AttackManager::registerFSM(AttackFSM* fsm) {
  team.push_back(fsm);
#ifdef USE_CANVAS_ATTACK_MANAGER
  Sorts::canvas.registerGob(fsm->getGob());
  Uint8 b = (Uint8) (((int) this) % 156) + 100;
  Sorts::canvas.setColor(fsm->getGob(), 0, 0, b);
#endif
}

void AttackManager::unregisterFSM(AttackFSM* fsm) {
  assert(find(team.begin(), team.end(), fsm) != team.end());
  team.erase(find(team.begin(), team.end(), fsm));
  if (fsm->target != NULL) {
    unassignTarget(fsm);
  }

#ifdef USE_CANVAS_ATTACK_MANAGER
  Sorts::canvas.unregisterGob(fsm->getGob());
#endif
  if (team.size() == 0) {
    msg << "I've gone out the window (Nobody cares about me anymore)" << endl;
    Sorts::amr->removeManager(this);
    delete this;
  }
}

void AttackManager::assignTarget(AttackFSM* fsm, SoarGameObject* target) {
  assert(targets.find(target) != targets.end());
  targets[target].assignAttacker(fsm);
  fsm->target = target;
}

void AttackManager::unassignTarget(AttackFSM* fsm) {
  assert(fsm->target != NULL);
  targets[fsm->target].unassignAttacker(fsm);
  fsm->target = NULL;
}

void AttackManager::unassignAll(SoarGameObject* target) {
  assert(targets.find(target) != targets.end());

  AttackTargetInfo& info = targets[target];
  for(set<AttackFSM*>::const_iterator
      i  = info.attackers_begin();
      i != info.attackers_end();
      ++i)
  {
    (*i)->target = NULL;
    (*i)->reassign = true;
  }
  info.unassignAll();
}

bool AttackManager::findTarget(AttackFSM* fsm) {
  msg << "FINDING A TARGET" << endl;
  GameObj* gob = fsm->getGob();
  for(int checkSaturated = 1; checkSaturated >= 0; checkSaturated--) {
  // try to hit immediately attackable things first
//  for(int checkSaturated = 1; checkSaturated >= 0; checkSaturated--) {
    for(vector<SoarGameObject*>::iterator
        i  = sortedTargets.begin();
        i != sortedTargets.end();
        ++i)
    {
      msg << "target: " << (*i)->getGob()->bp_name() << endl;
      if (canHit(gob, (*i)->getGob()) && 
          (checkSaturated == 0 || !targets[*i].isSaturated()))
      {
        assignTarget(fsm, *i);
#ifdef USE_CANVAS_ATTACK_MANAGER
        GameObj* tgob = (*i)->getGob();
        Sorts::canvas.trackDestination(gob, gobX(tgob), gobY(tgob));
#endif
        msg << "NEARBY TARG" << endl;
        return true;
      }
    }
//  }

  // now try to attack the "best" target
//  for(int checkSaturated = 1; checkSaturated >= 0; checkSaturated--) {
    for(vector<SoarGameObject*>::iterator
      i  = sortedTargets.begin();
      i != sortedTargets.end();
      ++i)
    {
      if (checkSaturated == 0 || !targets[*i].isSaturated()) {
        list<Vec2d> positions;
        attackArcPos(fsm->getGob(), (*i)->getGob(), positions);
        for(list<Vec2d>::iterator
            j  = positions.begin();
            j != positions.end();
            ++j)
        {
          if (fsm->move((*j)(0), (*j)(1)) == 0) {
            msg <<"Moving to Position: "<<(*j)(0)<<", "<<(*j)(1)<<endl;
            assignTarget(fsm, *i);
#ifdef USE_CANVAS_ATTACK_MANAGER
            GameObj* gob = (*i)->getGob();
            Sorts::canvas.trackDestination(fsm->getGob(),*gob->sod.x,*gob->sod.y);
#endif
            msg << "ARC TARG" << endl;
            return true;
          }
          else {
            msg << "ARC CHECK MOVE FAIL" << endl;
          }
        }
      }
    }
//  }

  // finally, just run toward someone, preferrably unsaturated
//  for(int checkSaturated = 1; checkSaturated >= 0; checkSaturated--) {
    for(vector<SoarGameObject*>::iterator
        i  = sortedTargets.begin();
        i != sortedTargets.end();
        ++i)
    {
      if (checkSaturated == 0 || !targets[*i].isSaturated()) {
        GameObj* gob = (*i)->getGob();
        if (fsm->move(*gob->sod.x, *gob->sod.y) == 0) {
          assignTarget(fsm, *i);
          msg << "LAST RESORT TARG" << endl;
          return true;
        }
      }
    }
//  }
  }
  return false;
}

// the current strategy is basically to focus fire on one enemy
// at a time until they're all dead, minimizing damage taken by self

// In the future, also implement running weak units away
int AttackManager::direct(AttackFSM* fsm) {
  msg << "NUMTARGS: " << targets.size() << endl;
  unsigned long st = gettime();
#ifdef USE_CANVAS_ATTACK_MANAGER
  Sorts::canvas.flashColor(fsm->getGob(), 0, 255, 0, 1);
  Sorts::canvas.update();
#endif

  GameObj* gob = fsm->getGob();
  SoarGameObject* sgob = Sorts::OrtsIO->getSoarGameObject(gob);

  if ( gob->dir_dmg > 0 && 
       ((double) gob->get_int("hp")) / gob->get_int("max_hp") < 0.2) 
  {
    
  }

  int numUnassigned = 0;
  for(list<AttackFSM*>::iterator
      i  = team.begin();
      i != team.end();
      ++i)
  {
    if ((*i)->target == NULL) {
      numUnassigned++;
    }
  }
  msg << "UNASSIGNED: " << numUnassigned << endl;


  if (updateTargetList() > 0) {
    msg << "UPDATE TARGET LIST" << endl;
    if (targets.size() == 0) {
      Sorts::amr->removeManager(this);
      delete this;
      return 1;
    }
    reprioritize();

    /*
    for(list<AttackFSM*>::iterator
        i  = team.begin();
        i != team.end();
        ++i)
    {
      if ((*i)->target != NULL) {
        unassignTarget(*i);
      }
    }
    */
  }

  if (fsm->target == NULL) {
    unsigned long st_find = gettime();
    if (!findTarget(fsm)) {
      msg << "FIND TARGET FAILED" << endl;
      fsm->failCount++;
      if (fsm->failCount > 10) {
        return -1;
      }
      return 0;
    }
    msg << "TIME TO FIND TARGET: " << (gettime() - st_find) / 1000 << endl;
  }
  
  assert(fsm->target != NULL);

  msg << "attacking target: " << fsm->target->getGob()->bp_name() << " " 
      << fsm->target->getGob() << endl;

  fsm->failCount = 0;

  GameObj* tgob = fsm->target->getGob();
  AttackTargetInfo& info = targets[fsm->target];

  if (!canHit(gob, tgob)) {
    if (fsm->isMoving()) {
      Vec2d dest = fsm->getDestination();
      if (canHit(gob, dest, tgob)) {
//        info.avgAttackerDistance();
//        double distToTarget 
//          = squaredDistance(gobX(gob), gobY(gob), gobX(tgob), gobY(tgob));
//        cout << "DIST: " << distToTarget << endl;
  //      if (distToTarget < info.avgAttackerDistance() * MAX_INDIVIDUAL_AHEAD ) {
          // if he's too far ahead of everyone, stop until others catch up
  //        fsm->stopMoving();
  //      }
  //      else {
          // everything's fine, keep going
          msg << "TIME SPENT: " << (gettime() - st) / 1000 << endl;
          return 0;
  //      }
      }
      else {
        msg << "xxx_dest can't hit\n";
      }
    }
    // not moving, or should be moving somewhere else
    msg << "CANNOT HIT" << endl;

    list<Vec2d> positions;
    attackArcPos(gob, tgob, positions);
    for(list<Vec2d>::iterator
        i  = positions.begin();
        i != positions.end();
        ++i)
    {
      if (fsm->move((*i)(0), (*i)(1)) == 0) {
        msg << "Moving to Position: " << (*i)(0) << ", " << (*i)(1) << endl;
#ifdef USE_CANVAS_ATTACK_MANAGER
//        Sorts::canvas.trackDestination(fsm->getGob(), (*i)(0), (*i)(1));
#endif
        break;
      }
      else {
        msg << "ARC CHECK MOVE FAIL 2" << endl;
      }
    }
    if (!fsm->isMoving()) {
      fsm->target = NULL;
    }
  }
  else if (!fsm->isFiring() ||  
           sgob->getLastAttacked() != fsm->target->getID())
  {
    fsm->attack(fsm->target);
  }

  msg << "TIME SPENT: " << (gettime() - st) / 1000 << endl;
  return 0;
}

int AttackManager::updateTargetList() {
  int numVanished = 0;
  for(map<SoarGameObject*, AttackTargetInfo>::iterator
      i =  targets.begin(); 
      i != targets.end();
      ++i)
  {
    if (!Sorts::OrtsIO->isAlive(i->first->getID())) {
      msg << "(" << (int) this << ") Unit " << i->first->getID() << " is no longer alive or moved out of view" << endl;

#ifdef USE_CANVAS_ATTACK_MANAGER
      // this target could have been in multiple attack managers
      if (Sorts::canvas.gobRegistered(i->first->getGob())) {
        Sorts::canvas.unregisterGob(i->first->getGob());
      }
#endif
      unassignAll(i->first);
      targets.erase(i);
      targetSet.erase(i->first);
      ++numVanished;
    }
  }
  return numVanished;
}

struct TargetCompare {
  Vec2d myPos;

  bool operator()(SoarGameObject* t1, SoarGameObject* t2) {
    // first always prefer those that are armed and shooting over 
    // those that are not
    ScriptObj* weapon1 = t1->getGob()->component("weapon");
    ScriptObj* weapon2 = t2->getGob()->component("weapon");
    if (weapon1 != NULL && weapon2 == NULL) {
      return true;
    }
    if (weapon1 == NULL && weapon2 != NULL) {
      return false;
    }
    
    if (weapon1 != NULL && weapon2 != NULL) {
      if (weapon1->get_int("shooting") == 1 && 
          weapon2->get_int("shooting") == 0) 
      {
        return true;
      }
      if (weapon1->get_int("shooting") == 0 && 
          weapon2->get_int("shooting") == 1) 
      {
        return false;
      }
    }

    // this formula was derived by minimizing damage to your own units,
    // and assuming that none of your units die while attacking (your
    // damage rate stays constant)
    double rating1 = weaponDamageRate(t1->getGob()) * t2->getGob()->get_int("hp");
    double rating2 = weaponDamageRate(t2->getGob()) * t1->getGob()->get_int("hp");

    if (rating1 < rating2) {
      return true;
    }
    else if (rating1 == rating2) { // break ties by distance
      Vec2d p1(*t1->getGob()->sod.x, *t1->getGob()->sod.y);
      Vec2d p2(*t2->getGob()->sod.x, *t2->getGob()->sod.y);

      double dist1 = (myPos - p1).magSq();
      double dist2 = (myPos - p2).magSq();

      if (dist1 < dist2) {
        return true;
      }
      else if (dist1 == dist2) {  // lexicographically break ties
        return (t1->getID() < t2->getID());
      }
    }
    return false;
  }
};

void AttackManager::reprioritize() {
  // calculate centroid
  double xsum = 0, ysum = 0;
  for(list<AttackFSM*>::iterator
      i  = team.begin();
      i != team.end();
      i++)
  {
    xsum += *(*i)->getGob()->sod.x;
    ysum += *(*i)->getGob()->sod.y;
  }
  
  TargetCompare comparator;
  comparator.myPos = Vec2d(xsum / team.size(), ysum / team.size());

  sortedTargets.clear();
  sortedTargets.insert(sortedTargets.end(), targetSet.begin(), targetSet.end());
  sort(sortedTargets.begin(), sortedTargets.end(), comparator);
}

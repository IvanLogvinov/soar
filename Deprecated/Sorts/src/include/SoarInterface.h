//#define GAME_ONE // TODO: rename this NO_SOAR, seperate out GAME_ONE
//#define GAME_FOUR // need also GAME_ONE

#ifndef SoarInterface_H
#define SoarInterface_H

#include<utility>
#include<list>
#include<map>
#include<pthread.h>

#include "sml_Client.h"

#include "SoarAction.h"
#include "general.h"
#include "FeatureMap.h"
#include "PerceptualGroupManager.h"

class Sorts;

class PerceptualGroup;

#ifndef GAME_ONE
using namespace sml;

// we need these attribute structs so we can remember the last value of the
// attribute, so that the input-link won't blink

struct InputLinkIntAttribute {
  IntElement* wme;
  int lastVal;
};

struct InputLinkFloatAttribute {
  FloatElement* wme;
  float lastVal;
};


struct InputLinkStringAttribute {
  StringElement* wme;
  string lastVal;
};

struct InputLinkGroupRep {
  int groupId;
  bool added;
  sml::Identifier* WMEptr;
  // change this later into hash_map<string, IntElement*> and write a hash
  // function for strings
  map<string, InputLinkIntAttribute> intProperties;
  map<string, InputLinkFloatAttribute> floatProperties;
  map<string, InputLinkStringAttribute> stringProperties;
};

typedef struct {
  sml::Identifier* identifierWME;
  sml::IntElement* sector0WME;
  sml::IntElement* sector1WME;
  sml::IntElement* sector2WME;
  sml::IntElement* sector3WME;
  sml::IntElement* sector4WME;
  sml::IntElement* sector5WME;
  sml::IntElement* sector6WME;
  sml::IntElement* sector7WME;
  sml::IntElement* sector8WME;
} InputLinkFeatureMapRep;


typedef struct {
  sml::Identifier* identifierWME;
  sml::IntElement* centerXWME;
  sml::IntElement* centerYWME;
  sml::IntElement* viewWidthWME;
  sml::IntElement* focusXWME;
  sml::IntElement* focusYWME;
  sml::IntElement* ownerGroupingWME;
  sml::IntElement* numObjectsWME;
  sml::IntElement* groupingRadiusWME;
} VisionParameterRep;

typedef struct {
  sml::Identifier* identifierWME;
  sml::StringElement* queryNameWME;
  sml::IntElement* param0WME;
  sml::IntElement* param1WME;
} QueryResultRep;

struct OAQueueStruct {
  ObjectAction action;
  sml::Identifier* wme;
  int gid;
};
struct AAQueueStruct {
  AttentionAction action;
  sml::Identifier* wme;
};
struct GAQueueStruct {
  GameAction action;
  sml::Identifier* wme;
};

/* 
The PerceptualGroupManager will have a pointer to this structure, and can call
the public functions to get new actions and change what groups Soar can
"see". Whatever needs to be done to initialize Soar or do higher level
things like pause for debugging will be done by the main loop, which can
call other (not yet defined) functions inside the SoarInterface.

SoarInterface will be responsible for creating and organizing the WMEs
for the groups, but not for determining what information is in the
structure (for example, we don't want a "health" WME for a tree, but
SoarInterface shouldn't need to figure that out). There will be a list
of visible properties inside the PerceptualGroups that will be set by the
PerceptualGroupManager and read by the SoarInterface to determine that.

Note: addGroup should not actually add the group to the input_link until
that group is refreshed! Initially, the stats will not be set.
*/

class SoarInterface {
  public:
    SoarInterface
    ( sml::Agent*      _agent,
      bool _oldAgent);

    ~SoarInterface();

    void getNewObjectActions(list<ObjectAction>& newActions);
    void getNewAttentionActions(list<AttentionAction>& newActions);
    void getNewGameActions(list<GameAction>& newActions);

    void updateViewFrame(int frame);

    // grouping commands for Group Manager to call
    void addGroup(PerceptualGroup* group);
    void removeGroup(PerceptualGroup* group);
    void refreshGroup(PerceptualGroup* group);
    int  groupId(PerceptualGroup* group);

    // feature map commands
    void addFeatureMap(FeatureMap* m, string name);
    void refreshFeatureMap(FeatureMap*, string name);
    
    // update player info
    void updatePlayerGold(int amount);
    void updatePlayerUnits(int, int, int);

    void updateMineralBuffer(int);

    void updateQueryResult(string name, int param0, int param1);
    /* this is the function for the Soar interrupt handler.
     * Don't try to call this
     */
    void getNewSoarOutput();

    void initSoarInputLink();
    void initVisionState(VisionParameterStruct vps);
    void updateVisionState(VisionParameterStruct& vps);
    bool getStale();
    void setStale(bool);

    void stopSoar();
    void startSoar();
    bool isSoarRunning() { return soarRunning; }

    int getSoarID(PerceptualGroup* grp);

  private:

    void processObjectAction(ObjectActionType, sml::Identifier*);
    void processAttentionAction(AttentionActionType, sml::Identifier*);
    void processGameAction(GameActionType, sml::Identifier*);
    void improperCommandError();

    // SML pointers
    sml::Agent *agent;
    bool oldAgent;

    sml::Identifier* inputLink;
    sml::Identifier* groupsIdWME;
    sml::Identifier* gameInfoIdWME;
    sml::IntElement* playerGoldWME;
    sml::IntElement* playerWorkersWME;
    sml::IntElement* playerMarinesWME;
    sml::IntElement* playerTanksWME;
    sml::IntElement* viewFrameWME;
    sml::IntElement* mineralBufferWME;

  /**************************************************
   *                                                *
   * Member variables for group management          *
   *                                                *
   **************************************************/

    // SoarInterface numbers each group based on its own convention
    int groupIdCounter;

    // these are the maps that keep track of input link <-> middleware objects
    /* Change these later to hash maps */
    map<PerceptualGroup*, InputLinkGroupRep> groupTable;
    map<int, PerceptualGroup*>               groupIdLookup;
   
  

  /**************************************************
   *                                                *
   * Member variables for feature maps              *
   *                                                *
   **************************************************/
    sml::Identifier* featureMapIdWME;
    map<string, InputLinkFeatureMapRep> featureMapTable;

    // for vision state

    VisionParameterStruct initialVisionParams;
    VisionParameterRep visionParamRep;

    QueryResultRep queryResultRep;

  /**************************************************
   *                                                *
   * Member variables for actions                   *
   *                                                *
   **************************************************/

    list<OAQueueStruct> objectActionQueue;
    list<AAQueueStruct> attentionActionQueue;
    list<GAQueueStruct> gameActionQueue;

    bool stale;
    bool soarRunning;
};
#else 
// fake SoarInterface for game1
class SoarInterface {
  public:
    SoarInterface() {};

    ~SoarInterface() {};

    void updateViewFrame(int frame) {}

    // grouping commands for Group Manager to call
    void addGroup(PerceptualGroup* group) {}
    void removeGroup(PerceptualGroup* group) {}
    void refreshGroup(PerceptualGroup* group) {}

    // feature map commands
    void addFeatureMap(FeatureMap* m, string name) {}
    void refreshFeatureMap(FeatureMap*, string name) {}
    
    // update player info
    void updatePlayerGold(int amount) { cout << "updated mineral count: " << amount << endl; }
    void updatePlayerUnits(int, int, int) {}

    void updateMineralBuffer(int) {}

    void updateQueryResult(string name, int param0, int param1) {}
    void initSoarInputLink() {}
    void initVisionState(VisionParameterStruct vps) {}
    void updateVisionState(VisionParameterStruct& vps) {}
    void stopSoar() {}
    void startSoar() {}
};

#endif
#endif

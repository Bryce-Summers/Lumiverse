#include <string>
#include "LumiverseCore.h"
#include "Cue.h"
#include "CueList.h"
#include "Layer.h"
#include "Playback.h"

#ifdef USE_ARNOLD
#include "Simulation/ArnoldAnimationPatch.h"
#endif

using namespace std;
using namespace Lumiverse;
using namespace Lumiverse::ShowControl;

int main(int argc, char**argv) {
  Rig rig("C:/Users/eshimizu/Documents/Lumiverse/Core/Lumiverse/data/movingLights_DMX.rig.json");

  Playback pb(&rig);
  pb.attachToRig();

  shared_ptr<Layer> layer1(new Layer(&rig, &pb, "layer1", 1));
  shared_ptr<Layer> layer2(new Layer(&rig, &pb, "layer2", 2));

  layer1->setMode(Layer::BLEND_OPAQUE);
  layer1->activate();
  layer2->activate();
  layer2->setOpacity(1);

  rig.init();

  DeviceSet inno = rig.query("inno");

  LumiverseColor* color = (LumiverseColor*)rig.getDevice("inno")->getParam("color");
  color->setxy(0.4, 0.4);

  inno.setParam("intensity", 0.0f);
  inno.setParam("shutter", "OPEN");
  inno.setParam("tilt", 135);
  inno.setParam("pan", 270);

  shared_ptr<Timeline> cue1(new Timeline());
  cue1->setKeyframe(&rig, 0, true);

  color->setxy(0.45f, 0.45f);
  inno.setParam("intensity", 1.0f);

  cue1->setKeyframe(&rig, 5000, false);

  shared_ptr<Timeline> cue2(new Timeline());
  cue2->setKeyframe(&rig, 0, true);
  color->setxy(0.3f, 0.3f);
  inno.setParam("pan", 302);
  inno.setParam("tilt", 120);
  cue2->setKeyframe(&rig, 5000, false);

  //Cue cue2(&rig, 3);
  //list1->storeCue(2, cue2);

  //inno.reset();
  //inno.setParam("pan", 0.5f);

  //Cue cue3(&rig);
  //list2->storeCue(1, cue3);

  // Add cue list to playback
  //pb.addCueList(list1);
  //pb.addCueList(list2);
  pb.addTimeline("cue1", cue1);
  pb.addTimeline("cue2", cue2);

  // Add layer to playback
  pb.addLayer(layer1);
  pb.addLayer(layer2);

  // Add cue list to layer

  // Prepare playback
  pb.start();
  rig.run();

  // Test keyframe insertion
  //DeviceSet chan1 = rig.query("#1");
  //chan1.setParam("intensity", 0.0f);
  //list1.getCue(1)->insertKeyframe(4, chan1);
  //list1.getCue(1)->insertKeyframe(5, chan1);

  //chan1.setParam("intensity", 1.0f);
  //list1.getCue(1)->insertKeyframe(4.5f, chan1);

  // Test keyframe overwrite
  //chan1.setParam("intensity", 0.0f);
  //list1.getCue(1)->insertKeyframe(4.5f, chan1);

  getchar();
  pb.getProgrammer()->clearAndReset();

  cout << "Layers ready.";
  getchar();
  layer1->play("cue1");
  getchar();
  layer1->play("cue2");
  //this_thread::sleep_for(chrono::seconds(5));

  while (1) {
    float val;
    rig["inno"]->getParam("intensity", val);
    cout << "inno Intensity: " << val << "\n";
    this_thread::sleep_for(chrono::milliseconds(10));
  }
}

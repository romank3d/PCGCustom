PCGCustom Plugin for UE 5.4
1. Unpack the contents of the ZIP archive to the engine plugins folder: .../UE_5.4x/Engine/Plugins/Marketplace, or add it to the project's plugins directory.
2. Find PCGCustom plugin under the Installed/Other plugins category, turn it on and restart the editor
3. In the PCG graph node pallette, start typing "PCGC"
4. Enjoy!

Ver 1.04
- Added "Discard Empty Data Sets Node" node, which removes data sets with no points, no attribute entries, or no composite data (Intersection, Difference, Union).

Ver 1.03
- "Get Actor Data Extended" node was updated to have 5.4 features and now using native PCG API for ActorSelector

Warning: If you're uprgading the plugin version in existing project and was using "Get Actor Data Extended" node in modes other than "Self", you'll need to set Actor Selector settings again.

Ver 1.02
- Added "Split Points" node - a custom version of the regular "Select Points" node with "Discarded Points" pin
- Multiple Fixes

Ver 1.01
- Added Grid shape for "SimpleShape" node
- Added pre-configured settings for "SimpleShape" node (shape modes appear as separate nodes in the node pallette)
- Additional settings for "SimpleShape" node
- Added "Get Actor Data Extended" node
- Added "Differnce By Tag" node

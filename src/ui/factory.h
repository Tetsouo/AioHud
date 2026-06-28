// factory.h -- create a widget instance from its layout `type` string.
//
// The single join point between the descriptor and the native widgets: the HUD reads
// each widget's `type` from layout.json and asks the factory for an instance. Adding a
// native widget = implement the class + add one line here under its `type` name.
#pragma once
#include <string>

namespace aio {

class Widget;
struct GameState;

// returns a NEW Widget for `type`, or nullptr if no native widget implements it yet
// (the HUD then skips that descriptor entry). The caller owns the returned pointer.
Widget* make_widget(const std::string& type, const GameState* state);

} // namespace aio

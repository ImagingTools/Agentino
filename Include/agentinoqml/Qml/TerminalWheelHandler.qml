import QtQuick 2.15

/**
	The wheel over the terminal when the program has asked for the mouse (?1000/?1002/?1003): it is reported to the program instead of
	scrolling the view. WheelHandler is a type of QtQuick 2.14, so it is in a file of its own and the pane keeps its import.
*/
WheelHandler {
	id: root;

	// direction: -1 the wheel rolled up (away from the user), 1 down.
	signal rolled(int direction);

	onWheel: {
		if (event.angleDelta.y > 0){
			root.rolled(-1);
		}
		else if (event.angleDelta.y < 0){
			root.rolled(1);
		}
	}
}

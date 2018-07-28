#ifndef O_VECTOR_H
#define O_VECTOR_H

class EulerVector3 { // orientation vector
public:
	EulerVector3(void) : val_yaw(0), val_pitch(0), val_roll(0) {}
	EulerVector3(float _yaw, float _pitch, float _roll) : val_yaw(_yaw), val_pitch(_pitch), val_roll(_roll) {}
	void set(float _yaw, float _pitch, float _roll) {
			val_yaw = _yaw; val_pitch = _pitch; val_roll = _roll;
	}
	float yaw() { return val_yaw; }
	float pitch() { return val_pitch; }
	float roll() { return val_roll; }
private:
	float val_yaw, val_pitch, val_roll;
};

#endif

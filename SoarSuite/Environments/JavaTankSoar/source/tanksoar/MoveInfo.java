package tanksoar;

import java.util.logging.*;

public class MoveInfo {
	private static Logger logger = Logger.getLogger("tanksoar");
	public boolean move;
	public int moveDirection;
	
	public boolean rotate;
	public String rotateDirection;
	
	public boolean fire;
	
	public boolean radar;
	public boolean radarSwitch;
	
	public boolean radarPower;
	public int radarPowerSetting;
	
	public boolean shields;
	public boolean shieldsSetting;
	
	public MoveInfo() {
		reset();
	}
	
	public void reset() {
		move = rotate = fire = radar = radarPower = shields = false;
	}
}


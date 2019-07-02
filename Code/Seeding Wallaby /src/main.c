#include <kipr/botball.h>
#include <drive.h>
#include <stdio.h>
#include <stdbool.h>

int camData[4];
int burning_building; //close building is 0, further building is 1 
int timeout; //if camera does not sense burning timeout will be set to 1
#define burning 0
#define claw 0
#define arm 1
#define arm_start 2000
#define arm_down 70
#define arm_up 2047
#define arm_mid 700
#define claw_open_people 530
#define claw_open_firetruck 1050
#define claw_open_medical 800
#define claw_open_fireman 1000
#define claw_close 100
#define claw_start 350
#define left_light 0
#define right_light 1
#define black 3600
#define white 2500
#define grey 3500
#define THRESHOLD 3870 //need to change to 3600 during the day, 3800 at night

int left_motor = 1;
int right_motor = 0;

double gyrocalibrate() {
    double changeGyroZ = 0;//find average gyro value when still
    int i = 0;
    for(i = 0; i < 2000; i++) {
        changeGyroZ += gyro_z();
    }
    changeGyroZ /= 2000;

    return changeGyroZ;
}

/*
void move(int speed, int time, int changeGyroZ) {//speed -100 to 100, time is in miliseconds
    int i = 0;
    int anglechange = 0;
    for(i = 0; i < time; i++) {//gyro move
        anglechange += gyro_z()-changeGyroZ;
        motor(MOT_LEFT, speed+anglechange/300);
        motor(MOT_RIGHT, speed-anglechange/300);
        msleep(1);
    }
    ao();
}*/

void move(int speed, int time, double changeGyroZ) {//speed -1500 to 1500, time is in miliseconds
    int i = 0;
    double anglechange = 0;
    for(i = 0; i < time; i++) {//gyro move
        anglechange += gyro_z()-changeGyroZ;
        mav(MOT_LEFT, speed+anglechange/10);
        mav(MOT_RIGHT, speed-anglechange/10);
        msleep(1);
    }
    ao();
}


/*void turn(int direction, int degree, int changeGyroZ) {
    int i = 0;
    int anglechange = 0;
    int x = degree*57000/90;
    if(direction == 1) { // 1 = counterclockwise, 0 = clockwise
        for(i = 0; i < 100; i++) {
            motor(0, 100);
            motor(1, -100);
            msleep(1);
        }
        while(anglechange > -x) {
            anglechange += gyro_z()-changeGyroZ;
            motor(0, 100);
            motor(1, -100);
            msleep(1);
        }
    } else {
        for(i = 0; i < 100; i++) {
            motor(0, -100);
            motor(1, 100);
            msleep(1);
        }
        while(anglechange < x) {
            anglechange += gyro_z()-changeGyroZ;
            motor(0, -100);
            motor(1, 100);
            msleep(1);
        }
    }
    ao();
}
*/
void turn(int direction, int degree, int changeGyroZ) {
    double anglechange = 0;
    int x = degree*270000/90;
    x /= 10;
    if(direction == 1) { // 1 = left, 0 = right
        while(anglechange > -x) {
            anglechange -= gyro_z()-changeGyroZ;
            motor(0, 100);
            motor(1, -100);
            msleep(10);
        }
    } else {
        while(anglechange < x) {
            anglechange -= gyro_z()-changeGyroZ;
            motor(0, -100);
            motor(1, 100);
            msleep(10);
        }
    }
    ao();
}

void updateCam(){
    camData[1] = camera_update();
    camData[2] = get_object_count(burning);
    if(camData[2] > 0){
        camData[3] = get_object_center_x(burning, camData[2]-1);
        camData[4] = get_object_center_y(burning, camData[2]-1);
        printf("Camera Update: %d, Objects: %d, x: %d, y: %d ", camData[1], camData[2], camData[3], camData[4]); 
    }
    else{
        printf("Camera Update: %d, Objects: %d ", camData[1], camData[2]);  
    }
}

void openCam(){
    int open = camera_open();
    if(open == 1){
        printf("Successfully opened\n");
    }
    else{
        printf("Failed to open\n");
        openCam();
    }
}

int findObject(){
    int i, objects, final, av = 0;  

    for(i = 1; i <= 35; i++){
        updateCam();
        printf("EMPTY\n");
    }

    for(i = 1; i <= 20; i++){
        updateCam();
        objects = camData[2];
        if(objects > 0){
            printf("Item detected\n");
            final++;
            av = av + objects;
        }
        else{
            printf("Item not detected\n");
        }
        msleep(20);
    }
    if(final > 0){
        av = av/final;
    }
    else{
        av = 0;
    }
    printf("Successes: %d, Average: %d\n", final, av);

    if(final >= 15){
        return 1;
    }
    else{
        return 0;
    }
}

void scanBuilding(){
    drive_clear();
    camera_open_black();
    timeout = 0;
    printf("scan start\n");
    while(get_object_count(burning) < 1 && timeout == 0){
        camera_update();
        //printf("burning: %d \n", get_object_count(burning)); 
        mav(MOT_LEFT, 950);
        mav(MOT_RIGHT, 900);
        msleep(100);
        if(gmpc(MOT_LEFT) > 1000){
            printf("scan time out\n");
            timeout = 1;
        }
    }
    ao();
    if(gmpc(MOT_LEFT) < 1000){
        burning_building = 0; 
        printf("burning building is close\n");
    }
    else{
        burning_building = 1;
        printf("burning building is far\n");
    }
    printf("left motor: %d", gmpc(MOT_LEFT));
    printf("right motor: %d", gmpc(MOT_RIGHT));
    camera_close();
    printf("scan done\n"); 
}

void line_follow(int dist, int speed) { 
    drive(speed, speed);
    long leftTarg = gmpc(MOT_LEFT) + 150 * dist;
    long rightTarg = gmpc(MOT_RIGHT) + 150 * dist;
    while (gmpc(MOT_RIGHT) < rightTarg || gmpc(MOT_LEFT) < leftTarg) {
        if (analog(left_light) > black)
            drive(speed * .75, speed * 1.25);
        else if (analog(right_light) > black)
            drive(speed * 1.25, speed * .75);
        else
            drive(speed, speed);
        msleep(1);
    }
    ao();
}

void line_follow_middle(int dist, int speed) {  
    drive(speed, speed);
    long leftTarg = gmpc(MOT_LEFT) + 150 * dist;
    long rightTarg = gmpc(MOT_RIGHT) + 150 * dist;
    while (gmpc(MOT_RIGHT) < rightTarg || gmpc(MOT_LEFT) < leftTarg) {
        if (analog(left_light) > grey)
            drive(speed * .85, speed * 1.15);
        else if (analog(right_light) > black)
            drive(speed * 1.15, speed * .85);
        else
            drive(speed, speed);
        msleep(1);
    }
    ao();
}

void line_sense(int speed) {    
    drive(speed * SPD_L_F / MAX_SPEED, speed * SPD_R_F / MAX_SPEED);
    while (analog(left_light) < black || analog(right_light) < black) {
        if (analog(right_light) > black)
            drive(speed * SPD_L_F / MAX_SPEED, 0);
        else if (analog(left_light) > black)
            drive(0, speed * SPD_R_F / MAX_SPEED);
        else if (analog(left_light) <= black && analog(right_light) <= black)
            drive(speed, speed);
        msleep(1);
    }
    printf("black line sensed\n");
    ao();
}

void line_sense_grey(int speed) {   
    drive(speed * SPD_L_F / MAX_SPEED, speed * SPD_R_F / MAX_SPEED);
    while (analog(left_light) < grey || analog(right_light) < grey) {
        if (analog(right_light) > grey){
            drive(speed * SPD_L_F / MAX_SPEED, 0);
        }
        else if (analog(left_light) > grey){
            drive(0, speed * SPD_R_F / MAX_SPEED);
        }    
        else if (analog(left_light) <= grey && analog(right_light) <= grey){
            drive(speed, speed);
        }
        msleep(1);
    }
    printf("grey line sensed\n");
    ao();
}

void slow_servo(int port, int pos) {

    while(get_servo_position(port) > pos) {
        set_servo_position(port, get_servo_position(port)-10);
        msleep(8);
    }

    while(get_servo_position(port) < pos) {
        set_servo_position(port, get_servo_position(port)+10);
        msleep(8);
    }
}

/*void slow_servo(int port, int start, int end) {
    int i = 0;
    for(i; i < 30; i++) {
        start += (end - start)/30;
        set_servo_position(port, start);        
        msleep(100);
    }
}
*/
void turn_left(int speed, int time){
    mav(MOT_LEFT, -speed);
    mav(MOT_RIGHT, speed);
    msleep(time);
    ao();
}

void turn_right(int speed, int time){
    mav(MOT_LEFT, speed);
    mav(MOT_RIGHT, -speed);
    msleep(time);
    ao();
}

void turn_left90(){
    mav(MOT_LEFT, -1500);
    mav(MOT_RIGHT, 1500);
    msleep(575);
    ao();
}

void turn_right90(){
    mav(MOT_LEFT, 1500);
    mav(MOT_RIGHT, -1500);
    msleep(615);
    ao();
}
/*
void calibrate_gyro()
{
    //takes the average of 50 readings
    int i = 0;
    double avg = 0;
    while(i < 50)
    {
        avg += gyro_z();
        msleep(1);
        i++;
    }
    bias = avg / 50.0;
    printf("New Bias: %f\n", bias);//prints out your bias. COMMENT THIS LINE OUT IF YOU DON'T WANT BIAS READINGS PRINTED OUT
}

//the same as calibrate_gyro() except faster to type
void cg()
{
    calibrate_gyro();
}

//turns the robot to reach a desired theta. 
//If you are expecting this function to work consistantly then don't take your turns too fast.
//The conversions from each wallaby to normal degrees varies but usually ~580000 KIPR degre80000 KIPR degrees = 90 degrees
void turn_with_gyro(int left_wheel_speed, int right_wheel_speed, double targetTheta)
{
    double theta = 0;//declares the variable that stores the current degrees
    mav(right_motor, right_wheel_speed);//starts the motors
    mav(left_motor, left_wheel_speed);
    //keeps the motors running until the robot reaches the desired angle
    while(theta < targetTheta)
    {
        msleep(10);//turns for .01 seconds
        theta += abs(gyro_z() - bias) * 10;//theta = omega(angular velocity, the value returned by gyroscopes) * time
       // printf("%d", theta);
    }

    //stops the motors after reaching the turn
    mav(right_motor, 0);
    mav(left_motor, 0);
}

//drives straight forward or backwards. The closer speed is to 0 the faster it will correct itself and the more consistent it will be but just do not go at max speed. Time is in ms. 
void drive_with_gyro(int speed, double time)
{ 
    double startTime = seconds();
    double theta = 0;
    while(seconds() - startTime < (time / 1000.0))
    {
        if(speed > 0)
        {
            mav(right_motor, (double)(speed - speed * (1.920137e-16 + 0.000004470956*theta - 7.399285e-28*pow(theta, 2) - 2.054177e-18*pow(theta, 3) + 1.3145e-40*pow(theta, 4))));  //here at NAR, we are VERY precise
            mav(left_motor, (double)(speed + speed * (1.920137e-16 + 0.000004470956*theta - 7.399285e-28*pow(theta, 2) - 2.054177e-18*pow(theta, 3) + 1.3145e-40*pow(theta, 4))));
        }
        else//reverses corrections if it is going backwards
        {
            mav(right_motor, (double)(speed + speed * (1.920137e-16 + 0.000004470956*theta - 7.399285e-28*pow(theta, 2) - 2.054177e-18*pow(theta, 3) + 1.3145e-40*pow(theta, 4)))); 
            mav(left_motor, (double)(speed - speed * (1.920137e-16 + 0.000004470956*theta - 7.399285e-28*pow(theta, 2) - 2.054177e-18*pow(theta, 3) + 1.3145e-40*pow(theta, 4))));
        }
        //updates theta
        msleep(10);
        theta += (gyro_z() - bias) * 10;
    }
}

void simple_drive_with_gyro(int speed, double time)
{
    double startTime = seconds(); //used to keep track of time
    double theta = 0; //keeps track of how much the robot has turned
    while(seconds() - startTime < time)
    {
        //if the robot is essentially straight then just drive straight
        if(theta < 1000 && theta > -1000)
        {
            mav(right_motor, speed);
            mav(left_motor, speed);
        }
        //if the robot is off to the right then drift to the left
        else if(theta < 1000)
        {
            mav(right_motor, speed + 100);
            mav(left_motor, speed - 100);
        }
        //if the robot is off to the left then drift to the right
        else
        {
            mav(right_motor, speed - 100);
            mav(left_motor, speed + 100);
        }
        //updates theta
        msleep(10);
        theta += (gyro_z() - bias) * 10;
    }
}

//keeps driving until detects black line
void gyro_light_stop(int speed)
{ 
    //double startTime = seconds();
    double theta = 0;
    while(analog(left_light) < THRESHOLD)
    {
        if(speed > 0)
        {
            mav(right_motor, (double)(speed - speed * (1.920137e-16 + 0.000004470956*theta - 7.399285e-28*pow(theta, 2) - 2.054177e-18*pow(theta, 3) + 1.3145e-40*pow(theta, 4))));  //here at NAR, we are VERY precise
            mav(left_motor, (double)(speed + speed * (1.920137e-16 + 0.000004470956*theta - 7.399285e-28*pow(theta, 2) - 2.054177e-18*pow(theta, 3) + 1.3145e-40*pow(theta, 4))));
        }
        else//reverses corrections if it is going backwards
        {
            mav(right_motor, (double)(speed + speed * (1.920137e-16 + 0.000004470956*theta - 7.399285e-28*pow(theta, 2) - 2.054177e-18*pow(theta, 3) + 1.3145e-40*pow(theta, 4)))); 
            mav(left_motor, (double)(speed - speed * (1.920137e-16 + 0.000004470956*theta - 7.399285e-28*pow(theta, 2) - 2.054177e-18*pow(theta, 3) + 1.3145e-40*pow(theta, 4))));
        }
        //updates theta
        msleep(10);
        theta += (gyro_z() - bias) * 10;
    }
}

//moves servo slowly
void slow_servo(int start, int end) {
    int i = 0;
    for(i; i < 30; i++) {
        start += (end - start)/30;
        set_servo_position(0, start);        
        msleep(100);
    }
}

void square_up_black_line () {
    while (analog(left_light) < THRESHOLD || analog(right_light) < THRESHOLD) {
        if(analog(left_light) < THRESHOLD && analog(right_light) < THRESHOLD) {
            mav(1, -200);
            mav(0, -200);
            msleep(1);
        }

        else if(analog(left_light) < THRESHOLD && analog(right_light) > THRESHOLD) {
            mav(1, 200);
            mav(0, -200);
            msleep(1);
        }
        else if (analog(left_light) > THRESHOLD && analog(right_light) < THRESHOLD) {
            mav(1, -200);
            mav(0, 200);
            msleep(1);
        } 
    }
    mav(0, 0);
    mav(1, 0);
}       

void square_up_black_line_2 () {
    while (LIGHT_0 < THRESHOLD || LIGHT_1 < THRESHOLD) {
        if(LIGHT_0 < THRESHOLD && LIGHT_1 < THRESHOLD) {
            mav(1, -150);
            mav(0, -150);
            msleep(1);
        }
        else if(LIGHT_0 < THRESHOLD && LIGHT_1 > THRESHOLD) {
            mav(1, 150);
            mav(0, -150);
            msleep(1);
        }
        else if (LIGHT_0 > THRESHOLD && LIGHT_1 < THRESHOLD) {
            mav(1, -150);
            mav(0, 150);
            msleep(1);
        } 
    }
    mav(0, 0);
    mav(1, 0);
}   
*/

int main(){
    //wallaby starts in the left corner against the back with claw closed facing towards create
    //servo starting positions
    set_servo_position(claw, claw_start);
    set_servo_position(arm, arm_start);
    double change = gyrocalibrate();
    enable_servos();
    shut_down_in(119);
    //people and ambulance go on ground of nonburning building, medical supplies go on top of nonburning building
    //firetruck goes on ground of burning building, firefighter goes on top of burning building 
    //turn_left90();
    move(-1500, 50, change);
    turn(1, 90, change); 
    msleep(500); 
    move(-1500, 400, change); //square up
    set_servo_position(arm, arm_down);
    msleep(1000);
    set_servo_position(claw, claw_open_medical);
    msleep(300);
    move(1500, 550, change);
    line_sense(500);
    //move(1500, 10, change); 
    //get medical supplies 
    //set_servo_position(claw, claw_close); //grabs medical supplies 1
    msleep(500);
    //set_servo_position(arm, arm_mid);
    move(1500, 40, change); 
    turn(1, 90, change);
    msleep(500);
    set_servo_position(arm, arm_down);
    msleep(300);
    slow_servo(claw, claw_open_medical);
    line_follow_middle(40, 1000); 
    slow_servo(claw, claw_close);
    while(analog(left_light) < black){
    	line_follow_middle(1, 600); 
    }
    move(-1500, 50, change); 
    //set_servo_position(arm, arm_mid);
    msleep(300);
    //turn_right90(); //turns right onto the black line by the building 
    turn(0, 90, change);
    msleep(500);
    //move(-1500, 175, change);
    //line_sense(-500); //backs up to prepare for camera  
    scanBuilding(); 
    //drop off medical supplies 
    if(burning_building == 1){
        msleep(500);
        turn(1, 90, change);//turns to face nonburning building
        msleep(500);
        move(1500, 75, change); 
        slow_servo(claw, claw_open_medical); //drops off medical supplies 
        msleep(500); 
        move(-1500, 165, change);
        msleep(500);
        slow_servo(arm, arm_up); 
        msleep(500); 
        turn(0, 90, change);
        msleep(500);
        line_sense(-500);//square on middle line
        msleep(200);
        move(-1500, 200, change); 
        msleep(200);
        turn(0, 90, change);
        msleep(500);
        move(-1500, 300, change); 
        msleep(100);
        turn(0, 90, change);//turn to face firetruck
        msleep(500);
    }else if(burning_building == 0){
        move(1500, 700, change); //moves forward to burning building 
        move(-1500, 25, change); 
        msleep(500);				
        //move(1500, 50, change);
        turn_left(1500, 300); 
        msleep(500);
        move(1500, 50, change); 
        slow_servo(claw, claw_open_medical); //drops off medical supplies 
        //set_servo_position(claw, claw_close);
        msleep(500);
        slow_servo(arm, arm_up); 
        msleep(500); 
        move(-1500, 300, change);
        turn_right(1500, 300);
        msleep(500);
        line_sense(-750); 
        msleep(500);
        turn(0, 90, change);
        msleep(500); 
        move(-1500, 420, change); 
        turn(0, 90, change); 
        msleep(500); 
    }
    //get firetruck
    line_sense(-500);
    msleep(100);
    move(1500, -200, change);
	msleep(200);
    slow_servo(claw, claw_open_firetruck);  
    msleep(500); 
    set_servo_position(arm, arm_down);
    msleep(500); 
    move(1500,400, change);
    msleep(500);
    slow_servo(claw, claw_close); //grabs firetruck 
    msleep(500); 
    //drop off firetruck
    if(burning_building == 1){
        printf("burning building 1");
        //turn_left90(); 
        turn(1, 90, change);
        msleep(200); printf("turn1");
        turn(1, 90, change);
        msleep(200); printf("turn2");
	    turn(1, 20, change);
        msleep(200); printf("turn3");
        move(1500, 100, change);
        msleep(500);
        slow_servo(claw, claw_open_firetruck); 
        msleep(500);
        slow_servo(arm, arm_up);
        
        turn(0, 20, change);  
  
    }else{
        printf("burning building 0");
        move(-1500, 200, change); 
        line_sense(-750); 
        //turn_left90();
        turn(1, 90, change);
        msleep(500);
        //left_turn(1500, 90); //turns to face forward 
        turn(1, 90, change);
        msleep(500);
        move(1500, 600, change); //moves to non burning building 
        turn(1, 40, change);
        msleep(300);
        slow_servo(claw, claw_open_firetruck); 
        msleep(500);
        move(-1500, 150, change);
        turn(0, 90, change);
        msleep(300);
        move(-1500, 200, change); 
    }
    //get fireman
    //turn_right90();
    //turn_right90();
    turn(0, 90, change);
    move(1500, 200, change);
    turn(1, 90, change);
    move(-1500, 1200, change); //square up
    move(1500, 500, change);
    turn(0, 90, change);
    set_servo_position(arm, arm_down);
    msleep(500);
    move(1500, 200, change);
    //turn_right90();
    set_servo_position(claw, claw_open_fireman); 
    move(1500, 300, change);
    msleep(500); 
    set_servo_position(claw, claw_close);
    msleep(500); 
    move(-1500, 150, change); 
    turn(1, 90, change);
    line_sense(500);
    turn(1, 90, change);
	line_follow_middle(25, 1200);
	set_servo_position(claw, 700);
	move(-1500, 400, change); 

    
    
 
    return 0;
}

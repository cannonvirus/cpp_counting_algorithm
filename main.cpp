#include <iostream>
#include <sstream>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <filesystem>
#include <variant>
#include <map>
#include <regex>

// opencv
#include "opencv2/opencv.hpp"
#include <opencv2/core/version.hpp>
#include <opencv2/imgcodecs.hpp>
#include <opencv2/calib3d/calib3d.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/core/core.hpp>

// rapid json
#include "rapidjson/document.h"
#include "rapidjson/writer.h"
#include "rapidjson/stringbuffer.h"
#include "rapidjson/filewritestream.h"
#include "rapidjson/prettywriter.h"
#include "rapidjson/istreamwrapper.h"
#include "rapidjson/ostreamwrapper.h"

// stat
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>


#include "my_json.h"

using namespace std;
using namespace rapidjson;
namespace fs = std::filesystem;

struct Lineinfo
{
    string name;
    string type;
    float xmin;
    float ymin;
    float width;
    float height;
    string direct_in;
    string direct_out;
};

struct Dotinfo
{
    float cx;
    float cy;
};

enum EntranceStatus 
{
    ENTRANCE_IN_AREA = 1,
    ENTRANCE_OUT_AREA = -1
};

enum RoomStatus 
{
    ROOM_IN_AREA = 1,
    ROOM_OUT_AREA = -1
};

enum ObjectStatus
{
    ROOM_IN_ENTRANCE_OUT_AREA = 1,
    ROOM_IN_ENTRANCE_IN_AREA = 2,
    ROOM_OUT_ENTRANCE_IN_AREA = 3,
    ROOM_OUT_ENTRANCE_OUT_AREA = 4
};

enum HallwayStatus
{
    HALLWAY_UP_AREA = 1,
    HALLWAY_DOWN_AREA = -1
};

float distanceCalculate(float x1, float y1, float x2, float y2)
{
    float x = x1 - x2; // calculating number to square in next step
    float y = y1 - y2;
    float dist;

    dist = pow(x, 2) + pow(y, 2); // calculating Euclidean distance
    dist = sqrt(dist);

    return dist;
}

void line_dataloader(string filepath, Lineinfo &data){
    map<string, variant<string, int, float>> line_dict;
    struct stat bff0;
    if (stat(filepath.c_str(), &bff0))
        cout << "No File" << endl;
    else
    {
        json_reader(filepath, line_dict, false);
        data.name = get<string>(line_dict["name"]);
        data.type = get<string>(line_dict["type"]);
        data.xmin = get<float>(line_dict["xmin"]);
        data.ymin = get<float>(line_dict["ymin"]);
        data.width = get<float>(line_dict["width"]);
        data.height = get<float>(line_dict["height"]);
        data.direct_in = get<string>(line_dict["direct_in"]);
        data.direct_out = get<string>(line_dict["direct_out"]);
        // cout << "File import success" << endl;
    }
}

int up_in_room(Lineinfo lineinfo, Dotinfo dotinfo){

    int status_in_entrance = 0;
    int status_in_room = 0;
    auto xmin = lineinfo.xmin;
    auto ymin = lineinfo.ymin;
    auto width = lineinfo.width;
    auto line_cx = (lineinfo.xmin + lineinfo.width / 2);
    auto line_cy = lineinfo.ymin;
    auto radius = lineinfo.width / 2;

    auto cx = dotinfo.cx;
    auto cy = dotinfo.cy;

    if (cx - xmin < width && cy < ymin) {
        status_in_room = RoomStatus::ROOM_IN_AREA;
    } else {
        status_in_room = RoomStatus::ROOM_OUT_AREA;
    }

    auto distance_door_center = distanceCalculate(line_cx, line_cy, cx, cy);
    if (distance_door_center < radius) {
        status_in_entrance = EntranceStatus::ENTRANCE_IN_AREA;
    } else {
        status_in_entrance = EntranceStatus::ENTRANCE_OUT_AREA;
    }

    if (status_in_room == RoomStatus::ROOM_IN_AREA && status_in_entrance == EntranceStatus::ENTRANCE_IN_AREA){
        return ObjectStatus::ROOM_IN_ENTRANCE_IN_AREA;
    } else if (status_in_room == RoomStatus::ROOM_IN_AREA && status_in_entrance == EntranceStatus::ENTRANCE_OUT_AREA){
        return ObjectStatus::ROOM_IN_ENTRANCE_OUT_AREA;
    } else if (status_in_room == RoomStatus::ROOM_OUT_AREA && status_in_entrance == EntranceStatus::ENTRANCE_IN_AREA){
        return ObjectStatus::ROOM_OUT_ENTRANCE_IN_AREA;
    } else {
        return ObjectStatus::ROOM_OUT_ENTRANCE_OUT_AREA;
    }
    
}

int down_in_room(Lineinfo lineinfo, Dotinfo dotinfo){

    int status_in_entrance = 0;
    int status_in_room = 0;
    auto xmin = lineinfo.xmin;
    auto ymin = lineinfo.ymin;
    auto width = lineinfo.width;
    auto line_cx = (lineinfo.xmin + lineinfo.width / 2);
    auto line_cy = lineinfo.ymin;
    auto radius = lineinfo.width / 2;

    auto cx = dotinfo.cx;
    auto cy = dotinfo.cy;

    if (cx - xmin < width && cy > ymin) {
        status_in_room = RoomStatus::ROOM_IN_AREA;
    } else {
        status_in_room = RoomStatus::ROOM_OUT_AREA;
    }

    auto distance_door_center = distanceCalculate(line_cx, line_cy, cx, cy);
    if (distance_door_center < radius) {
        status_in_entrance = EntranceStatus::ENTRANCE_IN_AREA;
    } else {
        status_in_entrance = EntranceStatus::ENTRANCE_OUT_AREA;
    }

    if (status_in_room == RoomStatus::ROOM_IN_AREA && status_in_entrance == EntranceStatus::ENTRANCE_IN_AREA){
        return ObjectStatus::ROOM_IN_ENTRANCE_IN_AREA;
    } else if (status_in_room == RoomStatus::ROOM_IN_AREA && status_in_entrance == EntranceStatus::ENTRANCE_OUT_AREA){
        return ObjectStatus::ROOM_IN_ENTRANCE_OUT_AREA;
    } else if (status_in_room == RoomStatus::ROOM_OUT_AREA && status_in_entrance == EntranceStatus::ENTRANCE_IN_AREA){
        return ObjectStatus::ROOM_OUT_ENTRANCE_IN_AREA;
    } else {
        return ObjectStatus::ROOM_OUT_ENTRANCE_OUT_AREA;
    }
    
}

HallwayStatus up_hallway(Lineinfo lineinfo, Dotinfo dotinfo){
    if (dotinfo.cx - lineinfo.xmin < lineinfo.width && dotinfo.cy > lineinfo.ymin) 
        return HallwayStatus::HALLWAY_UP_AREA;
    else 
        return HallwayStatus::HALLWAY_DOWN_AREA;
}

HallwayStatus down_hallway(Lineinfo lineinfo, Dotinfo dotinfo){
    if (dotinfo.cx - lineinfo.xmin < lineinfo.width && dotinfo.cy < lineinfo.ymin) 
        return HallwayStatus::HALLWAY_UP_AREA;
    else 
        return HallwayStatus::HALLWAY_DOWN_AREA;
}

HallwayStatus right_hallway(Lineinfo lineinfo, Dotinfo dotinfo){
    if (dotinfo.cy - lineinfo.ymin < lineinfo.height && dotinfo.cx > lineinfo.xmin) 
        return HallwayStatus::HALLWAY_UP_AREA;
    else 
        return HallwayStatus::HALLWAY_DOWN_AREA;
}

HallwayStatus left_hallway(Lineinfo lineinfo, Dotinfo dotinfo){
    if (dotinfo.cy - lineinfo.ymin < lineinfo.height && dotinfo.cx < lineinfo.xmin) 
        return HallwayStatus::HALLWAY_UP_AREA;
    else 
        return HallwayStatus::HALLWAY_DOWN_AREA;
}


bool room_status_by_line(Lineinfo lineinfo, Dotinfo dotinfo){

    int status_object;
    if (const vector<string> status = {"up", "down", "left", "right"}; lineinfo.direct_in == status[0]){
        status_object = up_in_room(lineinfo, dotinfo);
        cout << "object in room? [line1] : " << status_object << endl;
    } else if (lineinfo.direct_in == status[1]){
        status_object = down_in_room(lineinfo, dotinfo);
        cout << "object in room? [line2] : " << endl;
    } else if (lineinfo.direct_in == status[2]){
        cout << "left" << endl;
    } else {
        status_object = right_hallway(lineinfo, dotinfo);
        cout << "hallway [line3] : " << status_object << endl;
    }

    return true;
}

HallwayStatus hallway_status_by_line(Lineinfo lineinfo, Dotinfo dotinfo){

    HallwayStatus object_status;
    if (const vector<string> status = {"up", "down", "left", "right"}; lineinfo.direct_in == status[0]){
        object_status = up_hallway(lineinfo, dotinfo);
    } else if (lineinfo.direct_in == status[1]){
        object_status = down_hallway(lineinfo, dotinfo);
    } else if (lineinfo.direct_in == status[2]){
        object_status = left_hallway(lineinfo, dotinfo);
    } else {
        object_status = right_hallway(lineinfo, dotinfo);
    }
    cout << "type : " << lineinfo.type << " cx : " << dotinfo.cx << " cy : " << dotinfo.cy << " status : " << object_status << endl;
    return object_status;
}


int main()
{
    vector<Lineinfo> lineinfo_vec;

    for (const auto & file : fs::directory_iterator("/works/cpp_counting_algorithm/linedata")){

        string ext_file = fs::path(file.path()).extension();
        if (ext_file == ".json"){
            // cout << "file_path : " << file.path() << endl;
            Lineinfo lineparam;
            line_dataloader(file.path(), lineparam);
            lineinfo_vec.emplace_back(lineparam);
        }
    }

    Dotinfo exdot1{400,100};
    Dotinfo exdot2{400,250};
    Dotinfo exdot3{400,350};
    Dotinfo exdot4{950,140};

    // room
    room_status_by_line(lineinfo_vec[0], exdot1);
    room_status_by_line(lineinfo_vec[0], exdot2);
    room_status_by_line(lineinfo_vec[0], exdot3);
    room_status_by_line(lineinfo_vec[0], exdot4);

    // hallway 
    // if (lineinfo_vec[i].type == "hallway")
    hallway_status_by_line(lineinfo_vec[2], exdot1);
    hallway_status_by_line(lineinfo_vec[2], exdot2);
    hallway_status_by_line(lineinfo_vec[2], exdot3);
    hallway_status_by_line(lineinfo_vec[2], exdot4);

}

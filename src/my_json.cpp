#include "my_json.h"

using namespace std;
using namespace rapidjson;

template <class... Ts>
struct overloaded : Ts...
{
    using Ts::operator()...;
};
template <class... Ts>
overloaded(Ts...) -> overloaded<Ts...>;

void json_reader(string json_path, map<string, variant<string, int, float>> &notebook, bool print_option)
{
    std::ifstream ifs{json_path};
    if (!ifs.is_open())
    {
        std::cerr << "Could not open file for reading!\n";
    }
    IStreamWrapper isw{ifs};

    Document doc{};
    doc.ParseStream(isw);

    Value::MemberIterator M;

    for (M = doc.MemberBegin(); M != doc.MemberEnd(); M++)
    {
        // key = M->name.GetString();
        string key_str = M->name.GetString();
        if (M->value.IsString())
        {
            string value_str = M->value.GetString();
            notebook[key_str] = value_str;
        }
        else if (M->value.IsFloat())
        {
            float value_float = M->value.GetFloat();
            notebook[key_str] = value_float;
        }
        else if (M->value.IsInt())
        {
            int value_int = M->value.GetInt();
            notebook[key_str] = value_int;
        }
    }

    if (print_option)
    {
        cout << "==========================================================" << endl;
        for (const auto &[k, v] : notebook)
        {
            cout << k << " : ";
            visit([](const auto &x)
                  { cout << x; },
                  v);
            cout << '\n';
        }
        cout << "==========================================================" << endl;
    }
}

void json_writer(string json_path, map<string, variant<string, int, float>> &notebook)
{
    string component = "{";
    for (const auto &[k, v] : notebook)
    {
        component.append("\"");
        component.append(k);
        component.append("\":");

        visit(overloaded{[&component](int arg)
                         { component.append(to_string(arg)); },
                         [&component](double arg)
                         { component.append(to_string(arg)); },
                         [&component](const std::string &arg)
                         {  component.append("\"");
                            component.append(arg);
                            component.append("\""); }},
              v);
        component.append(", ");
    };
    component.erase(component.length() - 2, 2);
    component.append("}");

    Document d;
    d.Parse(component.c_str());

    FILE *fp = fopen(json_path.c_str(), "w");

    char writeBuffer[65536];
    FileWriteStream os(fp, writeBuffer, sizeof(writeBuffer));

    Writer<FileWriteStream> writer(os);
    d.Accept(writer);

    fclose(fp);
}

void dict_value_change(string key, variant<string, int, float> value, map<string, variant<string, int, float>> &notebook)
{
    visit(overloaded{[&notebook, &key](int arg)
                     { notebook[key].emplace<int>(arg); },
                     [&notebook, &key](double arg)
                     { notebook[key].emplace<float>(arg); },
                     [&notebook, &key](const std::string &arg)
                     { notebook[key].emplace<string>(arg); }},
          value);
}
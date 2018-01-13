#include <mysql_connection.h>
#include <cppconn/driver.h>
#include <cppconn/exception.h>
#include <cppconn/resultset.h>
#include <cppconn/statement.h>
#include <cppconn/prepared_statement.h>
#include <curl/curl.h>
#include <sstream>
#include <string>
#include <ctime>
#include <boost/property_tree/json_parser.hpp>
#include <boost/property_tree/ptree.hpp>
using namespace std;
using boost::property_tree::ptree;
using boost::property_tree::read_json;
using boost::property_tree::write_json;





static size_t WriteCallback(char *data, size_t size, size_t nmemb,
                std::string *writerData)
{
    if(writerData == NULL)
        return 0;

    writerData->append(data, size*nmemb);

    return size * nmemb;
}
string parseLastFromJson(string json){
    try {
        ptree pt2;
        istringstream is(json);

        read_json(is, pt2);
        return pt2.get<std::string>("last");
    }
    catch(std::exception const& e){
        return "";
    }

}
string getLast(const char* url){
    CURL *curl;
    string readBuffer;

    curl = curl_easy_init();
    curl_easy_setopt(curl, CURLOPT_URL, url);
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 1L);
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 1L);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &readBuffer);
    curl_easy_perform(curl);
    //cout << readBuffer << endl;
    curl_easy_cleanup(curl);

    return parseLastFromJson(readBuffer);
}
void toDatabase(int name, string last){

    try {


        sql::Driver *driver; // ustalanie sterownika z bazą
        sql::Connection *con; // konektor połączenia z bazą MySQL
        sql::PreparedStatement *pstmt; // wykonywanie polecenia SQL

        /* Create a connection */
        driver = get_driver_instance();
        con = driver->connect("tcp://127.0.0.1:3306", "root", "Qwerty12#");
        /* Connect to the MySQL test database */
        con->setSchema("bitbay"); // nazwa tabeli

        pstmt = con->prepareStatement("INSERT INTO rate(name, value) VALUES (?, ?)"); // istrukcje SQL. '?' zostaną zamienione na wartości podane w kolejnych poleceniach. czytane od lewej do prawej
        pstmt->setInt(1, name); // pierwy '?' w powyższym poleceniu SQL - relacja do kolumy z nazwami kryptowalut. wartosc "int"
        pstmt->setString(2, last); // drugi '?' w powyższym poleceniu SQL - wartość kursu
        pstmt->executeUpdate(); // wykonaj polecenie

        delete con; // usuwanie z pamieci
        delete pstmt; // usuwanie z pamieci

    } catch (sql::SQLException &e) {

        cout << "# ERR: SQLException in " << __FILE__;
        cout << "(" << __FUNCTION__ << ") on line_" << endl;
        cout << "# ERR: " << e.what();
        cout << " (MySQL error code: " << e.getErrorCode();
        cout << ", SQLState: " << e.getSQLState() << " )" << endl;
    }
}
int main()
{
    time_t time_begin;
    time_t time_end;

    const char* URL_GAME = "https://bitbay.net/API/Public/GAMEPLN/all.json";
    time( & time_begin ); // zapisanie czasu do zmiennej. 
    string last_game_tmp = getLast(URL_GAME); // pobranie aktualnego kursu waluty
    string last_game = ""; // zmienna pomocnicza

    do {

        time( & time_end ); // pobieranie aktualnego czasu w formie UNIX
        //cout << time_end << endl;
        last_game_tmp = getLast(URL_GAME); // funkcja zwraca kurs z serwisu BitBay

        if(last_game != last_game_tmp && !last_game_tmp.empty()){ // porównuje aktualny kurs z ostatnio pobranym. po przekroczeniu limitu zwracana jest pusta wartość
            // w przypadku gdy poprzednio pobrany kurs różni się z najnowszym...

            last_game = last_game_tmp; // nowy kurs jest zapisywany do zmiennej ze starym kursem

            toDatabase(2, last_game); // zapisuje do bazy najnowszy kurs. pierwszy arg. oznacza nazwe krypto. (1 - LSK, 2 - GAME). 

        sleep(5);

    }while (1/*(time_begin+3600 > time_end)*/); 


    return EXIT_SUCCESS;
}
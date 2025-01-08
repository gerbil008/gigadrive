#include <iostream>
#include <string_ops.h>
#include <nlohmann/json.hpp>
using json = nlohmann::json;


bool remove_entry_from_json_array(json& jsonArray, const std::string& entryToRemove) {
    // Überprüfen, ob die JSON-Struktur ein Array ist
    if (!jsonArray.is_array()) {
        throw std::runtime_error("Die JSON-Datenbank ist kein Array!");
    }

    // Iteriere über das Array, um den Eintrag zu finden
    for (auto it = jsonArray.begin(); it != jsonArray.end(); ++it) {
        if (*it == entryToRemove) { // Vergleich mit dem Eintrag
            jsonArray.erase(it);    // Entferne den Eintrag
            return true;            // Eintrag gefunden und entfernt
        }
    }

    return false; // Eintrag nicht gefunden
}

void delete_json(std::string filename){
    std::string fileContent = read_file("files.json");
    json jsonArray = json::parse(fileContent);


    // Entfernen des Eintrags
    if (remove_entry_from_json_array(jsonArray, filename)) {
        std::cout << "Eintrag erfolgreich entfernt." << std::endl;
    } else {
        std::cout << "Eintrag nicht gefunden." << std::endl;
    }

    // Ausgabe der aktualisierten JSON-Datenbank
    write_file("files.json", jsonArray.dump(4));
}

int main(void){
    delete_json("moin.txt");
}
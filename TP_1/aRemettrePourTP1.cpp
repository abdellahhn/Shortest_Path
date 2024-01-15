#include "DonneesGTFS.h"
#include <fstream>

using namespace std;



//! \brief ajoute les lignes dans l'objet GTFS
//! \param[in] p_nomFichier: le nom du fichier contenant les lignes
//! \throws logic_error si un problème survient avec la lecture du fichier
void DonneesGTFS::ajouterLignes(const std::string &p_nomFichier)
{
    //lire un ligne a la fois dans le files entré en parametre DONE
    std::fstream nwfiles(p_nomFichier, ios::in);

    if (!nwfiles.is_open()){
        std::cerr << "error loading the files" <<endl;
        return;
    }
    std::string line;
    int line_count = 0;


    while (std::getline(nwfiles, line)){
        if (line_count > 0){
            //for each line in the files we stock the lines data in a vector using the string_to_vector methode
            vector <string> route = (string_to_vector(line, ','));

            unsigned int p_id = stoi(route[0]);
            const std::string route_numero = route[2];
            const std::string route_description = route[4];
            const CategorieBus route_categorie = Ligne::couleurToCategorie(route[7]); // Assuming 'couleurToCategorie' is a static function

            //extract the important data in the vector and create an object line with the contructor ligne
            Ligne ligne = Ligne(route[0], route_numero, route_description, route_categorie);

            m_lignes.insert({route[0], ligne});
            m_lignes_par_numero.insert({route_numero, ligne});

        }
        line_count++;
    }
    nwfiles.close();
}

//! \brief ajoute les stations dans l'objet GTFS
//! \param[in] p_nomFichier: le nom du fichier contenant les stations
//! \throws logic_error si un problème survient avec la lecture du fichier
void DonneesGTFS::ajouterStations(const std::string &p_nomFichier)
{
    fstream stationFiles(p_nomFichier, ios::in);

    if (!stationFiles.is_open()){
        std::cerr<<"error loading the files"<<endl;
    }
    std::string station;
    int station_count = 0;

    while (std::getline(stationFiles, station)){
        if (station_count > 0){
            vector <string> station_vec = string_to_vector(station, ',');

            const Coordonnees p_coord = Coordonnees(stod(station_vec[4]), stod(station_vec[5]));

            Station stationO = Station(station_vec[0], station_vec[2], station_vec[3], p_coord);

            m_stations.insert({station_vec[0], stationO});
        }
        station_count++;
    }
    stationFiles.close();
}

//! \brief ajoute les transferts dans l'objet GTFS
//! \breif Cette méthode doit âtre utilisée uniquement après que tous les arrêts ont été ajoutés
//! \brief les transferts (entre stations) ajoutés sont uniquement ceux pour lesquelles les stations sont prensentes dans l'objet GTFS
//! \brief les transferts sont ajoutés dans m_transferts
//! \brief les from_station_id des stations de m_transferts sont ajoutés dans m_stationsDeTransfert
//! \param[in] p_nomFichier: le nom du fichier contenant les station
//! \throws logic_error si un problème survient avec la lecture du fichier
//! \throws logic_error si tous les arrets de la date et de l'intervalle n'ont pas été ajoutés
void DonneesGTFS::ajouterTransferts(const std::string &p_nomFichier)
{
    fstream transferFile(p_nomFichier, ios::in);
    std::string transfer;
    if (!m_tousLesArretsPresents){
        std::cerr<<"Tout les arret de la date n'ont pas ete ajouté"<<endl;
    }

    if (!transferFile.is_open()){
        std::cerr<<"error loading the files"<<endl;
    }

    int transferCount = 0;

    while(std::getline(transferFile, transfer)){
        if (transferCount > 0){
            vector <string> tranfer_V = string_to_vector(transfer, ',');

            const string min_transfer_time = tranfer_V[3];
            if (tranfer_V[3] == "0"){
                tranfer_V[3] = "1";
            }
            if ((m_stations.find(tranfer_V[0]) != m_stations.end())
                and (m_stations.find(tranfer_V[1]) != m_stations.end())){

                m_transferts.push_back(make_tuple(tranfer_V[0], tranfer_V[1], stoi(tranfer_V[3])));
                m_stationsDeTransfert.insert(tranfer_V[0]);
            }
        }
        transferCount++;

    }
    transferFile.close();
}


//! \brief ajoute les services de la date du GTFS (m_date)
//! \param[in] p_nomFichier: le nom du fichier contenant les services
//! \throws logic_error si un problème survient avec la lecture du fichier
void DonneesGTFS::ajouterServices(const std::string &p_nomFichier)
{
    //open files and see if problem happens etc...
    fstream serviceFile(p_nomFichier, ios::in);
    if (!serviceFile.is_open()){
        std::cerr<<"error loading the files"<<endl;
    }
    //loop through the .txt file and select only the <1> exception type
    std::string services;
    int service_count = 0;

    while (std::getline(serviceFile, services)){
        if (service_count > 0){
            vector <string> services_V = string_to_vector(services, ',');

            std::string p_date = services_V[1];
            unsigned int an = stoul(p_date.substr(0, 4));
            unsigned int mois = stoul(p_date.substr(4, 2));
            unsigned int jour = stoul(p_date.substr(6, 2));

            if (services_V[2] == "1" && Date(an, mois, jour) == m_date){
                m_services.insert(services_V[0]);
            }
        }
        service_count++;
    }
    serviceFile.close();
}

//! \brief ajoute les voyages de la date
//! \brief seuls les voyages dont le service est présent dans l'objet GTFS sont ajoutés
//! \param[in] p_nomFichier: le nom du fichier contenant les voyages
//! \throws logic_error si un problème survient avec la lecture du fichier
void DonneesGTFS::ajouterVoyagesDeLaDate(const std::string &p_nomFichier)
{
    fstream VoyageFiles(p_nomFichier, ios::in);

    if (!VoyageFiles.is_open()){
        std::cerr<<"error loading the files"<<endl;
    }
    std::string service;
    int count = 0;

    while (std::getline(VoyageFiles, service)){
        if (count > 0){
            vector <string> station_vec = string_to_vector(service, ',');

            const string p_id = (station_vec[0]);
            std::string p_ligne = station_vec[3];
            std::string p_serv = station_vec[1];
            std::string p_dst = station_vec[4];

            unsigned int year = stoul(p_serv.substr(0,4));
            unsigned int month = stoul(p_serv.substr(4,2));
            unsigned int d = stoul(p_serv.substr(6,2));
            Date date = Date(year, month, d);

            for (auto it = m_services.begin(); it != m_services.end(); it++){
                if (it->compare(p_serv) == 0){
                    Voyage voyage = Voyage(p_ligne, p_id, p_serv, p_dst);
                    m_voyages.insert({p_ligne, voyage});
                }
            }

        }
        count++;
    }
    VoyageFiles.close();
}

//! \brief ajoute les arrets aux voyages présents dans le GTFS si l'heure du voyage appartient à l'intervalle de temps du GTFS
//! \brief Un arrêt est ajouté SSI son heure de départ est >= now1 et que son heure d'arrivée est < now2
//! \brief De plus, on enlève les voyages qui n'ont pas d'arrêts dans l'intervalle de temps du GTFS
//! \brief De plus, on enlève les stations qui n'ont pas d'arrets dans l'intervalle de temps du GTFS
//! \param[in] p_nomFichier: le nom du fichier contenant les arrets
//! \post assigne m_tousLesArretsPresents à true
//! \throws logic_error si un problème survient avec la lecture du fichier
void DonneesGTFS::ajouterArretsDesVoyagesDeLaDate(const std::string &p_nomFichier)
{
    fstream files(p_nomFichier, ios::in);
    if (!files.is_open()){
        cerr<<"Error loading the files"<<endl;
    }
    std::string date;
    int lineCount = 0;

    while(getline(files, date)){
        if (lineCount > 0){
            vector <string> vectorr = string_to_vector(date, ',');


            int hour_arv = stoi(vectorr[1].substr(0,2));
            int min_arv = stoi(vectorr[1].substr(3,2));
            int sec_arv = stoi(vectorr[1].substr(6,2));

            int hour_dep = stoi(vectorr[2].substr(0,2));
            int min_dep = stoi(vectorr[2].substr(3,2));
            int sec_dep = stoi(vectorr[2].substr(6,2));



            if (m_voyages.find(vectorr[0]) != m_voyages.end()){
                Heure heure_ar = Heure(hour_arv, min_arv, sec_arv);
                Heure heure_dep = Heure(hour_dep, min_dep, sec_dep);

                if (m_now2 > heure_ar and heure_dep >= m_now1){
                    Arret::Ptr a_ptr = make_shared<Arret>(vectorr[3], heure_ar, heure_dep, stoi(vectorr[4]), vectorr[0]);
                    m_voyages[vectorr[0]].ajouterArret(a_ptr);
                    m_stations[vectorr[3]].addArret(a_ptr);
                    ++m_nbArrets;
                }
            }
        }
        lineCount++;
    }
    for (auto it = m_voyages.begin(); it != m_voyages.end();) {
        if( it -> second.getNbArrets() == 0)
        {
            it = m_voyages.erase(it);
        }
        else
        {
            ++it;
        }
    }

    for (auto itt = m_stations.begin(); itt != m_stations.end(); ) {
        if( itt -> second.getNbArrets() == 0)
        {
            itt = m_stations.erase(itt);
        }

        else
        {
            ++itt;
        }
    }
    files.close();

    m_tousLesArretsPresents = true;
}




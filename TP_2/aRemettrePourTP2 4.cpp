

#include "ReseauGTFS.h"
#include <sys/time.h>

using namespace std;

//! \brief ajout des arcs dus aux voyages
//! \brief insère les arrêts (associés aux sommets) dans m_arretDuSommet et m_sommetDeArret
//! \throws logic_error si une incohérence est détecté lors de cette étape de construction du graphe
void ReseauGTFS::ajouterArcsVoyages(const DonneesGTFS &gtfs) {
    try {
        for (const auto &tripPair: gtfs.getVoyages()) {
            m_origine_dest_ajoute = false;
            const auto &arrets = tripPair.second.getArrets();

            for (const auto &currentArret: arrets) {
                m_sommetDeArret[currentArret] = m_arretDuSommet.size();

                if (m_origine_dest_ajoute) {
                    auto origine = m_sommetDeArret[currentArret] - 1;
                    auto destination = m_sommetDeArret[currentArret];
                    auto weight = currentArret->getHeureArrivee() - m_arretDuSommet.back()->getHeureArrivee();


                    m_leGraphe.ajouterArc(origine, destination, weight);
                }

                m_arretDuSommet.push_back(currentArret);
                m_origine_dest_ajoute = true;
            }
        }
    } catch (const exception &E) {
        cerr << "Erreur: une incohérence est détecté lors de l'ajout des arcs voyages" << E.what() << endl;
    }
}


//! \brief ajouts des arcs dus aux transferts entre stations
//! \throws logic_error si une incohérence est détecté lors de cette étape de construction du graphe
void ReseauGTFS::ajouterArcsTransferts(const DonneesGTFS &gtfs) {
    try {
        for (const auto &transfert : gtfs.getTransferts()) {
            const auto &arretsStationA = gtfs.getStations().at(get<0>(transfert)).getArrets();
            const auto &arretsStationB = gtfs.getStations().at(get<1>(transfert)).getArrets();

            for (const auto &arretA : arretsStationA) {
                string ligneA = gtfs.getVoyages().at(arretA.second->getVoyageId()).getLigne();

                for (auto arretB = arretsStationB.lower_bound(arretA.second->getHeureArrivee().add_secondes(get<2>(transfert)));
                     arretB != arretsStationB.end(); ++arretB) {
                    auto poids = arretB->first - arretA.second->getHeureArrivee();
                    unsigned int tempsMin = get<2>(transfert);

                    if (poids >= tempsMin) {
                        vector<string> lignesUniques = {gtfs.getLignes().at(ligneA).getNumero()};
                        string ligneB = gtfs.getVoyages().at(arretB->second->getVoyageId()).getLigne();
                        if (find(lignesUniques.begin(), lignesUniques.end(), gtfs.getLignes().at(ligneB).getNumero()) == lignesUniques.end()) {
                            m_leGraphe.ajouterArc(m_sommetDeArret[arretA.second], m_sommetDeArret[arretB->second], poids);
                            const string &bon_numero = gtfs.getLignes().at(ligneB).getNumero();


                            lignesUniques.push_back(bon_numero);
                        }
                    }
                }
            }
        }
    } catch (const exception &e) {
        cerr << "Erreur : Une incohérence est détectée lors de ajouterArcsTransferts" << e.what() << endl;
    }
}


//! \brief ajouts des arcs d'une station à elle-même pour les stations qui ne sont pas dans DonneesGTFS::m_stationsDeTransfert
//! \throws logic_error si une incohérence est détecté lors de cette étape de construction du graphe
void ReseauGTFS::ajouterArcsAttente(const DonneesGTFS &gtfs) {
    try {
        for (const auto &station : gtfs.getStations()) {
            const auto &stationId = station.first;

            if (gtfs.getStationsDeTransfert().find(stationId) != gtfs.getStationsDeTransfert().end()) continue;

            map<string, vector<Arret::Ptr>> lignesArrets;

            for (const auto &arret : station.second.getArrets()) {
                const auto &voyageId = arret.second->getVoyageId();
                const auto &ligneId = gtfs.getVoyages().at(voyageId).getLigne();
                lignesArrets[ligneId].push_back(arret.second); //
            }

            for (const auto &ligneArrets : lignesArrets) {
                for (size_t i = 0; i < ligneArrets.second.size(); ++i) {
                    for (size_t j = i + 1; j < ligneArrets.second.size(); ++j) {
                        unsigned int poids = ligneArrets.second[j]->getHeureArrivee() - ligneArrets.second[i]->getHeureArrivee();

                        if (poids >= delaisMinArcsAttente) {
                            m_leGraphe.ajouterArc(m_sommetDeArret[ligneArrets.second[i]],
                                                  m_sommetDeArret[ligneArrets.second[j]], poids);
                        }
                    }
                }
            }
        }
    } catch (const exception &e) {
        cerr << "Erreur lors de l'ajout des arcs d'attente : " << e.what() << endl;
    }
}



//! \brief ajoute des arcs au réseau GTFS à partir des données GTFS
//! \brief Il s'agit des arcs allant du point origine vers une station si celle-ci est accessible à pieds et des arcs allant d'une station vers le point destination
//! \param[in] p_gtfs: un objet DonneesGTFS
//! \param[in] p_pointOrigine: les coordonnées GPS du point origine
//! \param[in] p_pointDestination: les coordonnées GPS du point destination
//! \throws logic_error si une incohérence est détecté lors de la construction du graphe
//! \post constuit un réseau GTFS représenté par un graphe orienté pondéré avec poids non négatifs
//! \post assigne la variable m_origine_dest_ajoute à true (car les points orignine et destination font parti du graphe)
//! \post insère dans m_sommetsVersDestination les numéros de sommets connctés au point destination
void ReseauGTFS::ajouterArcsOrigineDestination(const DonneesGTFS &gtfs, const Coordonnees &pointOrigine, const Coordonnees &pointDestination) {
    try {
        set<string> uniqueLines;
        unsigned int weight;
        size_t vertexI, vertexJ;

        auto addStop = [this](const string& stationId, const Heure& heureDepart, const Heure& heureArrivee, int sommetId) {
            auto stop = make_shared<Arret>(stationId, heureDepart, heureArrivee, sommetId, "nom");
            m_arretDuSommet.push_back(stop);
            m_sommetDeArret.emplace(stop, m_arretDuSommet.size() - 1);
            return m_sommetDeArret[stop];
        };

        m_sommetOrigine = addStop(stationIdOrigine, Heure(0, 0, 0), Heure(1, 0, 0), 1);
        m_sommetDestination = addStop(stationIdDestination, Heure(0, 0, 0), Heure(1, 10, 0), 2);
        m_leGraphe.resize(m_leGraphe.getNbSommets() + 2);
        m_nbArcsOrigineVersStations = 0;
        vertexI = m_sommetOrigine;

        auto addArcs = [this, &uniqueLines, &gtfs, &vertexI, &weight](const Station& station, double distance) {
            weight = static_cast<unsigned int>((distance / vitesseDeMarche) * 3600);
            for (auto stopIter = station.getArrets().lower_bound(gtfs.getTempsDebut().add_secondes(weight));
                 stopIter != station.getArrets().end(); ++stopIter) {
                auto stop = stopIter->second;
                string lineNumber = gtfs.getLignes().at(gtfs.getVoyages().at(stop->getVoyageId()).getLigne()).getNumero();
                if (uniqueLines.insert(lineNumber).second) {
                    weight = static_cast<unsigned int>(stopIter->first - gtfs.getTempsDebut());
                    size_t vertexJ = m_sommetDeArret[stop];
                    m_leGraphe.ajouterArc(vertexI, vertexJ, weight);
                    ++m_nbArcsOrigineVersStations;
                }
            }
        };

        for (const auto& stationPair: gtfs.getStations()) {
            double distance = pointOrigine - stationPair.second.getCoords();
            if (distance <= distanceMaxMarche) {
                addArcs(stationPair.second, distance);
                uniqueLines.clear();
            }
        }

        m_sommetsVersDestination.clear();
        m_nbArcsStationsVersDestination = 0;
        vertexJ = m_sommetDestination;

        for (const auto& stationPair: gtfs.getStations()) {
            double distToDest = stationPair.second.getCoords() - pointDestination;
            if (distToDest <= distanceMaxMarche) {
                for (const auto& pairStationArret: stationPair.second.getArrets()) {
                    vertexI = m_sommetDeArret[pairStationArret.second];
                    weight = static_cast<unsigned int>((distToDest / vitesseDeMarche) * 3600);
                    m_leGraphe.ajouterArc(vertexI, vertexJ, weight);
                    m_sommetsVersDestination.push_back(vertexI);
                    ++m_nbArcsStationsVersDestination;
                }
            }
        }

        m_origine_dest_ajoute = true;
    } catch (const exception &e) {
        cerr << "Erreur lors de l'ajout des arcs origine-destination : " << e.what() << endl;
    }
}


//! \brief Remet ReseauGTFS dans l'était qu'il était avant l'exécution de ReseauGTFS::ajouterArcsOrigineDestination()
//! \param[in] p_gtfs: un objet DonneesGTFS
//! \throws logic_error si une incohérence est détecté lors de la modification du graphe
//! \post Enlève de ReaseauGTFS tous les arcs allant du point source vers un arrêt de station et ceux allant d'un arrêt de station vers la destination
//! \post assigne la variable m_origine_dest_ajoute à false (les points orignine et destination sont enlevés du graphe)
//! \post enlève les données de m_sommetsVersDestination
void ReseauGTFS::enleverArcsOrigineDestination() {
    try {
        for (const size_t destination_arc: m_sommetsVersDestination) {
            m_leGraphe.enleverArc(destination_arc, m_sommetDestination);
        }
        m_sommetsVersDestination.clear();

        unsigned long siz = m_leGraphe.getNbSommets() - 2;
        m_leGraphe.resize(siz);

        m_nbArcsOrigineVersStations = 0;
        m_nbArcsStationsVersDestination = 0;

        if (!m_arretDuSommet.empty()) {
            auto lastArret1 = m_arretDuSommet.back();
            m_sommetDeArret.erase(lastArret1);
            m_arretDuSommet.pop_back();

            if (!m_arretDuSommet.empty()) {
                auto lastArret2 = m_arretDuSommet.back();
                m_sommetDeArret.erase(lastArret2);
                m_arretDuSommet.pop_back();
            }
        }

        m_origine_dest_ajoute = false;
    }
    catch (exception &E) {
        cerr << "Erreur : Une incohérence est détecté lors de la modification du graphe" << E.what() << endl;
    }
}
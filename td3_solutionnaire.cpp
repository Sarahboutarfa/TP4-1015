#pragma region "Includes"
#define _CRT_SECURE_NO_WARNINGS 
#include "structures_solutionnaire_td3.hpp"      

#include "bibliotheque_cours.hpp"
#include "verification_allocation.hpp"
#include <iostream>
#include <fstream>
#include <string>
#include <limits>
#include <algorithm>
#include <sstream>
#include <vector>
#include "cppitertools/range.hpp"
#include "gsl/span"
#include "debogage_memoire.hpp"       
using namespace std;
using namespace iter;
using namespace gsl;

#pragma endregion

typedef uint8_t UInt8;
typedef uint16_t UInt16;

#pragma region "Fonctions de base pour lire le fichier binaire"
template <typename T>
T lireType(istream& fichier)
{
	T valeur{};
	fichier.read(reinterpret_cast<char*>(&valeur), sizeof(valeur));
	return valeur;
}
#define erreurFataleAssert(message) assert(false&&(message)),terminate()
static const uint8_t enteteTailleVariableDeBase = 0xA0;
size_t lireUintTailleVariable(istream& fichier)
{
	uint8_t entete = lireType<uint8_t>(fichier);
	switch (entete) {
	case enteteTailleVariableDeBase+0: return lireType<uint8_t>(fichier);
	case enteteTailleVariableDeBase+1: return lireType<uint16_t>(fichier);
	case enteteTailleVariableDeBase+2: return lireType<uint32_t>(fichier);
	default:
		erreurFataleAssert("Tentative de lire un entier de taille variable alors que le fichier contient autre chose à cet emplacement.");  //NOTE: Il n'est pas possible de faire des tests pour couvrir cette ligne en plus du reste du programme en une seule exécution, car cette ligne termine abruptement l'exécution du programme.  C'est possible de la couvrir en exécutant une seconde fois le programme avec un fichier films.bin qui contient par exemple une lettre au début.
	}
}

string lireString(istream& fichier)
{
	string texte;
	texte.resize(lireUintTailleVariable(fichier));
	fichier.read((char*)&texte[0], streamsize(sizeof(texte[0])) * texte.length());
	return texte;
}

#pragma endregion

void ListeFilms::changeDimension(int nouvelleCapacite)
{
	Film** nouvelleListe = new Film*[nouvelleCapacite];
	
	if (elements != nullptr) {
		nElements = min(nouvelleCapacite, nElements);
		for (int i : range(nElements))
			nouvelleListe[i] = elements[i];
		delete[] elements;
	}
	
	elements = nouvelleListe;
	capacite = nouvelleCapacite;
}

void ListeFilms::ajouterFilm(Film* film)
{
	if (nElements == capacite)
		changeDimension(max(1, capacite * 2));
	elements[nElements++] = film;
}


span<Film*> ListeFilms::enSpan() const { return span(elements, nElements); }

void ListeFilms::enleverFilm(const Film* film)
{
	for (Film*& element : enSpan()) {  
		if (element == film) {
			if (nElements > 1)
				element = elements[nElements - 1];
			nElements--;
			return;
		}
	}
}

shared_ptr<Acteur> ListeFilms::trouverActeur(const string& nomActeur) const
{
	for (const Film* film : enSpan()) {
		for (const shared_ptr<Acteur>& acteur : film->acteurs.enSpan()) {
			if (acteur->nom == nomActeur)
				return acteur;
		}
	}
	return nullptr;
}

shared_ptr<Acteur> lireActeur(istream& fichier, const ListeFilms& listeFilms)
{
	Acteur acteur = {};
	acteur.nom            = lireString(fichier);
	acteur.anneeNaissance = int(lireUintTailleVariable (fichier));
	acteur.sexe           = char(lireUintTailleVariable(fichier));

	shared_ptr<Acteur> acteurExistant = listeFilms.trouverActeur(acteur.nom);
	if (acteurExistant != nullptr)
		return acteurExistant;
	else {
		cout << "Création Acteur " << acteur.nom << endl;
		return make_shared<Acteur>(move(acteur));  
	}
}

Film* lireFilm(istream& fichier, ListeFilms& listeFilms)
{
	Film* film = new Film;
	film->titre       = lireString(fichier);
	film->realisateur = lireString(fichier);
	film->annee = int(lireUintTailleVariable(fichier));
	film->recette     = int(lireUintTailleVariable(fichier));
	auto nActeurs = int(lireUintTailleVariable(fichier));
	film->acteurs = ListeActeurs(nActeurs);  
	cout << "Création Film " << film->titre << endl;

	for ([[maybe_unused]] auto i : range(nActeurs)) {  
		film->acteurs.ajouter(lireActeur(fichier, listeFilms));
	}

	return film;
}

ListeFilms creerListe(string nomFichier)
{
	ifstream fichier(nomFichier, ios::binary);
	fichier.exceptions(ios::failbit);
	
	int nElements = int(lireUintTailleVariable(fichier));

	ListeFilms listeFilms;
	for ([[maybe_unused]] int i : range(nElements)) {
		listeFilms.ajouterFilm(lireFilm(fichier, listeFilms));
	}
	
	return listeFilms;
}


void ListeFilms::detruire(bool possedeLesFilms)
{
	if (possedeLesFilms)
		for (Film* film : enSpan())
			delete film;
	delete[] elements;
}

ostream& operator<< (ostream& os, const Acteur& acteur)
{
	return os << "  " << acteur.nom << ", " << acteur.anneeNaissance << " " << acteur.sexe << endl;
}
	
ostream& operator<< (ostream& os, const Film& film)
{
	os << "Titre: " << film.titre << endl;
	os << "  Réalisateur: " << film.realisateur << "  Année :" << film.annee << endl;
	os << "  Recette: " << film.recette << "M$" << endl;

	os << "Acteurs:" << endl;
	for (const shared_ptr<Acteur>& acteur : film.acteurs.enSpan())
		os << *acteur;
	return os;
}

ostream& operator<< (ostream& os, const ListeFilms& listeFilms)
{
	static const string ligneDeSeparation = "\033[32m────────────────────────────────────────\033[0m\n";
	os << ligneDeSeparation;
	for (const Film* film : listeFilms.enSpan()) {
		os << *film << ligneDeSeparation;
	}
	return os;
}


//fonction pour assigner les attributs
void extraireDansFichier(const string& ligne, Livre& livre){
	istringstream iss(ligne);
    getline(iss, livre.titre, '\t');
    iss >> livre.annee;
    iss.ignore(numeric_limits<streamsize>::max(), '\t'); 
    iss >> livre.millionsCopiesVendues;
    iss.ignore(numeric_limits<streamsize>::max(), '\t'); 
    iss >> livre.nombrePages;
}


//fonction pour lire livres.txt et ajouter dans une biblio
void lireTxt(const string& nomFichier, vector<shared_ptr<Item>>& biblio){
		ifstream fichier(nomFichier);
		if (!fichier){
			cout << "impossible d'ouvrir le fichier"<< endl;
			return;
		}
		string ligne;
		while (getline(fichier, ligne)){
			Livre livre;
			extraireDansFichier(ligne, livre);
			biblio.push_back(make_shared<Livre>(livre));
		}
	fichier.close();
}


//fonction qui permet d'afficher la bibliotheque

void afficherBiblio(const vector<shared_ptr<Item>>& biblio) {
    cout << "Bibliothèque : " << endl;
    cout << "--------------------------------------" << endl;
    for (const auto& itemPtr : biblio) {
        if (const auto filmPtr = dynamic_pointer_cast<Film>(itemPtr)) {
            cout << "Type: Film" << endl;
            cout << "Titre: " << filmPtr->titre << endl;
            cout << "Réalisateur: " << filmPtr->realisateur << endl;
            cout << "Année: " << filmPtr->annee << endl;
            cout << "Recette: " << filmPtr->recette << "M$" << endl;
            cout << "--------------------------------------" << endl;
        } else if (const auto livrePtr = dynamic_pointer_cast<Livre>(itemPtr)) {
            cout << "Type: Livre" << endl;
            cout << "Titre: " << livrePtr->titre << endl;
            cout << "Auteur: " << livrePtr->auteur << endl;
            cout << "Année: " << livrePtr->annee << endl;
            cout << "Millions de copies vendues: " << livrePtr->millionsCopiesVendues << endl;
            cout << "Nombre de pages: " << livrePtr->nombrePages << endl;
            cout << "--------------------------------------" << endl;
        } else {
            cout << "Type d'élément inconnu" << endl;
        }
    }
}


int main()
{
	#ifdef VERIFICATION_ALLOCATION_INCLUS
	bibliotheque_cours::VerifierFuitesAllocations verifierFuitesAllocations;
	#endif
	bibliotheque_cours::activerCouleursAnsi();  

	static const string ligneDeSeparation = "\n\033[35m════════════════════════════════════════\033[0m\n";

	ListeFilms listeFilms = creerListe("films.bin");
	
	//TP4: faire un vecteur
	vector<shared_ptr<Item>> bibliotheque;

	//TP4: transferer les films de listeFilm dans le vecteur
	for (Film* filmPtr: listeFilms.enSpan()){
    bibliotheque.push_back(make_shared<Film>(*filmPtr));
	}

	//detruire listeFilms car ne sert plus a rien
	listeFilms.detruire(true);

	//lire livres.txt et mettre mettre les livres dans la biblio
	lireTxt("livres.txt", bibliotheque);

	//afficher biblio

	afficherBiblio(bibliotheque);


}

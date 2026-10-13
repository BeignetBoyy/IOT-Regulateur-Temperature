# JSON Validator

Ce script python permet de vérifier qu’un fichier JSON généré par ESP32 respecte un schéma JSON strict.

La validation se déroule en deux étapes :
1. Vérifier que le fichier contient un JSON syntaxiquement correct.
2. Vérifier que les données respectent le schéma défini dans `schema.json`.



## Fonctionnalités

Le programme permet de :

- Charger le schéma JSON depuis un fichier.
- Vérifier que le schéma est lui-même valide.
- Charger les données JSON fournies par l'ESP32.
- Détecter les erreurs de syntaxe JSON.
- Vérifier la présence de tous les champs obligatoires.
- Contrôler le type des valeurs (object, string, number, integer, boolean, etc.).
- Vérifier les contraintes définies dans le schéma comme les valeurs autorisées et les limites numériques.
- Refuser les champs supplémentaires lorsque additionalProperties vaut false.
- Afficher l'ensemble des erreurs de conformité, avec le chemin permettant de localiser chaque problème.
- Retourner True si le fichier est valide et False en cas d'erreur.


## Dépendances

- python
- jsonschema

### Installation

Dans un terminal, exécuter :

```bash
pip install jsonschema
```

---

## Utilisation

### Lancer la validation

Si les fichiers validate.py, esp.json et schema.json se trouvent dans le même répertoire, exécuter :

```bash
python validate.py
```

Le programme utilise alors les fichiers indiqués dans le bloc principal de validate.py :

```python
if __name__ == "__main__":
    validate_json("esp.json", "schema.json")
```

Le premier argument = fichier à valider. Le second = la référence.

Pour changer les fichiers utilisés, il suffit de modifier les noms passés à validate_json().

---

## Fonctionnement du programme

Le programme commence par charger le schéma JSON et les données de l'ESP32. Il vérifie d'abord que les deux fichiers sont corrects et que le schéma respecte les règles de JSON Schema.

Draft202012Validator(schema) crée un validateur à partir du schéma. La méthode iter_errors(data) compare les données au schéma et récupère toutes les erreurs détectées.

Les erreurs sont ensuite triées avec sorted() selon leur chemin (absolute_path). Ce tri permet d'afficher les erreurs dans un ordre plus lisible pour l'utilisateur.

Pour chaque erreur le programme construit un chemin : $ est la racine du JSON, .nom_du_champ est la propriété et [index] est un élément de tableau. 

Si des erreurs sont présentes, la fonction retourne False. Sinon, elle affiche un message de réussite et retourne True.

---

## Intégration dans Node-RED

Le fichier validate_stdin.py permet d'utiliser le validateur directement dans le flow Node-RED 

Il reçoit les données JSON directement depuis l'entrée standard (stdin). Donc pas nécessaire d'enregistrer chaque message reçu dans un fichier esp.json avant de le valider.

Les deux scripts utilisent le même fichier schema.json :
- validate.py sert à effectuer des tests manuels sur un fichier JSON.
- validate_stdin.py sert à valider les messages reçus par Node-RED.

---


### Configuration du nœud Exec

Dans la configuration du nœud Exec, renseigner la commande permettant de lancer Python et le script. Par exemple :

```bash
python -u "C:\chemin\vers\validate_stdin.py"
```

Remplacer C:\chemin\vers\validate_stdin.py par le chemin réel du validate_stdin.py sur l'ordinateur de l'utilisateur.

Le mode d'exécution doit permettre au script de recevoir les messages sur son entrée standard (stdin). La version de validate_stdin.py utilisée doit également être compatible avec le mode choisi dans le nœud Exec.


## Schéma JSON attendu

Le fichier schema.json définit la structure autorisée des données de l'ESP32.

Le schéma utilisé dans ce projet comprend les objets suivants :

- status
- location
- regul
- info
- net
- reporthost

Les champs obligatoires, les types, les valeurs autorisées et les éventuelles limites numériques sont définis directement dans schema.json. Le schéma constitue la référence utilisée par le validateur.

### Rôle des principales contraintes

| Contrainte | Rôle |
|---|---|
| type | Définit le type de donnée accepté. |
| required | Liste les propriétés obligatoires d'un objet. |
| enum | Limite une valeur à une liste de valeurs autorisées. |
| minimum / maximum | Définit les limites numériques inclusives. |
| additionalProperties: false | Interdit les propriétés non déclarées dans l'objet concerné. |
| $schema | Indique la version de JSON Schema utilisée. |

additionalProperties: false marche uniquement à l'objet dans lequel il est déclaré. 

---

## Exemples de résultats

```text
JSON VALIDE : syntaxe et schema corrects.
```

```text
JSON INVALIDE : erreur de syntaxe ligne 5, colonne 3 : ...
```

```text
JSON NON CONFORME au schema :
- $ : 'status' is a required property
```

```text
JSON NON CONFORME au schema :
- $.regul.temperature : 'abc' is not of type 'number'
```


```text
JSON NON CONFORME au schema :
- $.status : Additional properties are not allowed ('extra_field' was unexpected)
```


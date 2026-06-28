# Brief design — AioHUD : widget Party / Alliances (style XivParty)

> À coller dans une conversation **claude.ai** (design) **avec les 3 images** :
> `party.png`, `alliances.png`, `party_alliances_group.png`.
> Objectif : challenger nos choix et proposer la meilleure direction visuelle **dans nos contraintes**.

---

## 1. Contexte

Refonte native (C++ / Windower 4) d'un HUD pour **Final Fantasy XI**. Ce composant = la liste de **party + alliances**, inspiré de **XivParty** (look FFXIV) mais adapté FFXI. C'est une **maquette HTML/CSS** servant de spec visuelle avant l'implémentation native.

Structure XivParty (vérifiée dans la source) : **3 listes séparées** =

- **liste 0 = ta party** (1 à 6 membres)
- **liste 1 = alliance 1** (0 à 6)
- **liste 2 = alliance 2** (0 à 6)
→ jusqu'à **18 joueurs**, empilés en colonne (alliances **au-dessus** de la party).

## 2. Ce que le jeu fournit (contraintes DURES — non négociables)

- **Ta party (6) :** HP/MP/TP (valeurs réelles), **buffs**, **action en cours** (sort/JA — texte seulement, pas de barre de cast fiable pour les autres), **job + sous-job**.
- **Membres d'alliance (12) :** **HP/MP/TP uniquement**. Le jeu **ne transmet PAS** leurs buffs ni leurs actions, et **pas le sous-job** (le sub vient du paquet party 0x0DD = ta party only).
- **Flags de rôle** (paquet d'alliance, connus pour tous) : `isAllianceLeader`, `isLeader` (party leader), `isQuarterMaster`.
- **TP** : 0 → **3000** (max). Seuil **1000** = peut Weaponskill.
- **HP** : couleur par palier (vert >75 %, jaune 50-75, orange 25-50, rouge <25). 0 HP = KO.
- **Noms** : **3 à 15 caractères** (prévoir le max).
- Cible (le membre qu'on target), claim adverse, etc.

## 3. Contraintes d'intégration

- **Fond opaque obligatoire** : la box doit **cacher la party/alliance native** du jeu derrière.
- Doit cohabiter avec l'**UI native** (zones interdites : menu haut, équipement, chat, etc.) — placée en bas-droite par défaut.
- **Déplaçable / configurable** par l'utilisateur (effectif 0-6 par liste, etc.).

## 4. Objectifs / ce qu'on veut

- **Moderne, classe, ambiance FFXI heroic-fantasy**, sans tomber dans le « template web » (pas de barres de couleur gadget à gauche, etc.).
- **Compact** (18 joueurs doivent tenir sans monter en haut de l'écran) **MAIS lisible**.
- **Lecture instantanée** : voir d'un coup d'œil qui est ma party vs alliance 1 vs alliance 2.
- **Mettre en avant le joueur principal** (toi).
- **Rôles compréhensibles instantanément** (leader alliance / leader party / quarter master).
- Cohérence de taille : les 3 box **même largeur**, lignes **même hauteur** (qu'un membre cast ou non, qu'il ait 0 ou 3 rôles).

## 5. État actuel (cf. images) — nos choix

- **Barres HP/MP/TP horizontales** (fines, valeur centrée + contour noir, border de la couleur de la jauge : HP vert / MP bleu / TP magenta). HP<25 % clignote rouge, TP≥1000 clignote magenta.
- **Badge de job** compact teinté par rôle (tank bleu / healer vert / dd rouge / support jaune), main + sub (party), main seul (alliance).
- **Marqueurs de rôle** = icônes vectorielles **horizontales** à gauche du badge : 👑 couronne **or** (alliance leader), ★ étoile **or** (party leader), **$ vert** (quarter master). Largeur réservée fixe → alignement constant.
- **Séparation entre les 3 box** : fenêtres encadrées + coins internes carrés + un divider net à la jointure quand elles se collent (snap). Pas d'en-tête texte. Pas d'ombre portée entre box collées.
- **Buffs** de la party : hors de la box, à gauche de chaque ligne.
- **Cible** surlignée (liseré or + curseur triangulaire). **KO** = ligne rouge/inactive (pas d'icône).
- Fond opaque dégradé indigo + halo magique + glint doré (FFXI).

## 6. Questions sur lesquelles on veut ton avis

1. **Hiérarchie visuelle des 3 listes** : comment rendre « ma party vs alliance 1 vs 2 » évident sans en-tête texte ni gadget — la solution actuelle (encadrement + jointure) est-elle la meilleure ? Alternatives ?
2. **Différencier party (riche : buffs/cast/sub) vs alliance (HP/MP/TP only)** sans que ça paraisse incohérent ou cassé.
3. **Mettre en avant le joueur principal** de façon élégante (autre que jauges plus hautes ?).
4. **Marqueurs de rôle** : icônes/couleurs/placement optimaux pour 0→3 rôles cumulés sans déformer la ligne ni gaspiller d'espace.
5. **Compacité vs lisibilité** : meilleur compromis pour 18 joueurs à l'écran (barres horizontales vs jauges rondes vs autre).
6. **Ambiance FFXI heroic-fantasy moderne** : palette, matière, profondeur — comment pousser le côté « classe » sans nuire à la lisibilité en jeu (sur un fond de jeu chargé).
7. Tout ce qu'on n'aurait pas vu.

## 7. Rappels de format

- Lignes très compactes (~23 px), 18 lignes possibles → densité forte.
- Sur fond de jeu **chargé/coloré** → la box doit rester lisible (d'où l'opacité + contraste).
- Pas d'emoji dans l'UI finale (vraies icônes vectorielles).

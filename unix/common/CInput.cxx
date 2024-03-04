

#include <list>
#include <rfb/LogWriter.h>

#include "CInput.h"


static rfb::LogWriter vlog("CInput");

/*************************************
 * TODO
 * - Double check "changes" struct type
 * - Double check all "changes" are retrieved/propagated to server (name &  map)
 */

/************************* Custom key list ****************************/
typedef struct {
    KeyCode keyCode;
    KeySym keySym;
}AssignedKey;


/**
 * Values are stored in 'most recently used' order
 */
static std::list<AssignedKey> assignedKeys;


static Bool isAssignmentStillValid(AssignedKey assignment, XkbDescPtr xkb)
{
    return XkbKeyNumGroups(xkb, assignment.keyCode) > 0 &&
        XkbKeySymsPtr(xkb, assignment.keyCode)[0] == assignment.keySym &&
        (xkb->names == NULL || xkb->names->keys[assignment.keyCode].name[0] == 'T');
}

static void addAssignment(KeyCode code, KeySym sym)
{
    vlog.info("Setting custom key assignment: [%d] -> %ld", code, sym);
    assignedKeys.push_front({ code, sym });
}

/**
 * Returns the least recently used and still valid assignment.
 * If no valid assignment found, returns an asssignment with keyCode = 0.
 * Returned and invalid assignments are removed from list.
 */
static AssignedKey getReusableAssignment(XkbDescPtr xkb)
{
    while (!assignedKeys.empty()) {
        AssignedKey last = assignedKeys.back();
        assignedKeys.pop_back();
        if (isAssignmentStillValid(last, xkb))
            return last;
        else
            vlog.info("Assingnment no longer valid: %d", last.keyCode);
    }
    return { 0, 0 };
}



static KeyCode getUnusedKey(XkbDescPtr xkb)
{
    for (KeyCode key = xkb->max_key_code; key >= xkb->min_key_code; key--)
        if (XkbKeyNumGroups(xkb, key) == 0)
            return key;
    return 0;
}

static void assignKeysymToKey(KeySym keysym, KeyCode key, XkbDescPtr xkb, XkbChangesPtr changes)
{
    int types[1];
    KeySym* syms;
    KeySym upper, lower;

    // Assign a custom name
    if (xkb->names) {
        xkb->names->keys[key].name[0] = 'T';
        xkb->names->keys[key].name[1] = '0' + (key / 100) % 10;
        xkb->names->keys[key].name[2] = '0' + (key / 10) % 10;
        xkb->names->keys[key].name[3] = '0' + (key / 1) % 10;
        changes->names.changed |= XkbKeyNamesMask;
        changes->names.first_key = key;
        changes->names.num_keys = 1;
    }

    // Update  group
    XConvertCase(keysym, &lower, &upper);
    if (upper == lower)
        types[XkbGroup1Index] = XkbOneLevelIndex;
    else
        types[XkbGroup1Index] = XkbAlphabeticIndex;

    XkbChangeTypesOfKey(xkb, key, 1, XkbGroup1Mask, types, &changes->map);

    // Assign symbols
    syms = XkbKeySymsPtr(xkb, key);
    if (upper == lower)
        syms[0] = keysym;
    else {
        syms[0] = lower;
        syms[1] = upper;
    }

    changes->map.changed |= XkbKeySymsMask;
    changes->map.first_key_sym = key;
    changes->map.num_key_syms = 1;

    // Remember assignment
    addAssignment(key, syms[0]);
}

KeyCode addKeysymToMap(KeySym keysym, XkbDescPtr xkb, XkbChangesPtr changes)
{
    //vlog.info("Trying to add unknown keysym: %ld", keysym);

    KeyCode key = getUnusedKey(xkb);
    if (!key) {
        key = getReusableAssignment(xkb).keyCode;
        vlog.info("Got reusable key: %d", key);
    }
    if (!key)
        return 0;

    assignKeysymToKey(keysym, key, xkb, changes);

    return key;
}

void removeAddedKeysymsFromMap(XkbDescPtr xkb, XkbMapChangesPtr changes) {
    KeyCode lowestKeyCode = xkb->max_key_code;
    KeyCode highestKeyCode = xkb->min_key_code;

    AssignedKey assignment = getReusableAssignment(xkb);
    while (assignment.keyCode) {
        KeyCode keyCode = assignment.keyCode;

        XkbChangeTypesOfKey(xkb, keyCode, 0, XkbGroup1Mask, NULL, changes);

        if (keyCode < lowestKeyCode)
            lowestKeyCode = keyCode;

        if (keyCode > highestKeyCode)
            highestKeyCode = keyCode;

        // Get next
        assignment = getReusableAssignment(xkb);
    }

    // Did we actually find something to remove?
    if (highestKeyCode >= lowestKeyCode) {
        changes->changed |= XkbKeySymsMask;
        changes->first_key_sym = lowestKeyCode;
        changes->num_key_syms = highestKeyCode - lowestKeyCode + 1;
    }
}


void onKeyUsed(KeyCode usedKey) {
    if (assignedKeys.empty())
        return;

    // If there a custom assignment for this key, move it to the front
    std::list<AssignedKey>::iterator it = assignedKeys.begin();
    for (; it != assignedKeys.end(); ++it) {
        AssignedKey assignment = *it;
        if (assignment.keyCode == usedKey && it != assignedKeys.begin()) {
            assignedKeys.erase(it);
            assignedKeys.push_front(assignment);
            break;
        }
    }
}

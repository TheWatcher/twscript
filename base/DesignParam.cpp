
#include <cctype>
#include <cstring>
#include <cstdlib>
#include "QVarWrapper.h"
#include "DesignParam.h"
#include "ScriptLib.h"

/* ------------------------------------------------------------------------
 *  DesignParam
 */

bool DesignParam::get_param_string(const std::string& design_note, std::string& parameter)
{
    // Use Telliamed's code to fetch the string. If it isn't set, just return false.
    // FIXME: Can this be replaced with something cleaner?
    char *value = GetParamString(design_note.c_str(), fullname.c_str(), NULL);
    if(!value) return false;

    // Update the parameter value store; there's no need to retain the source string
    parameter = value;
    g_pMalloc -> Free(value);

    return true;
}


/* ------------------------------------------------------------------------
 *  DesignParamString
 */

bool DesignParamString::init(const std::string& design_note, const std::string& default_value)
{
    // For string parameters, all we need to do is fetch the parameter as a string; no
    // validation or processing is needed beyond this.
    bool valid = get_param_string(design_note, data);

    // Allow for fallback if the parameter is not set in the design note
    if(!valid) data = default_value;

    is_set(valid);

    // note that init returns true on success
    return true;
}


/* ------------------------------------------------------------------------
 *  DesignParamFloat
 */

bool DesignParamFloat::init(const std::string& design_note, const float default_value, const bool add_listeners)
{
    std::string param;

    bool valid = get_param_string(design_note, param);
    if(valid) {
        valid = data.init(param, default_value, add_listeners);
    }

    return valid;
}


/* ------------------------------------------------------------------------
 *  DesignParamTime
 */

namespace {
    /** Try to parse the specified param string as a time string.
     *  A time string is a number (float or int) optionally followed
     *  by 's' (to indicate that the time is in seconds) or 'm' (for
     *  minutes). The value is converted to an int before placing
     *  it in the provided store variable.
     *
     * @param param A string containing the time string to parse
     * @param store A reference to an int to store the result in
     * @return true if the string contains a recognised time string,
     *         false if it appears not to (either it contains nothing
     *         the function can pars,e or it appears to contain more
     *         characters than a time string should).
     */
    bool is_modified_time(const std::string& param, int& store)
    {
        const char* str = param.c_str();
        char* end;

        // Parse as a float, as '0.5s' or '2.5m' are valid times
        float val = strtof(str, &end);

        // Nothing parsed?
        if(end == str) {
            return false;
        // convert seconds
        } else if(tolower(*end) == 's') {
            val *= 1000.0f;
        // convert minutes
        } else if(tolower(*end) == 'm') {
            val *= 60000.0f;
        // FIXME: do we want to support 'h' here ?

        // any other content following the value may indicate
        // that this is a qvar calc rather than a fixed time
        } else if(*end) {
            store = static_cast<int>(val);
            return false;
        }

        store = static_cast<int>(val);
        return true;
    }
}


bool DesignParamTime::init(const std::string& design_note, int default_value, const bool add_listeners)
{
    std::string param;

    // Fetch the raw string from the design note
    bool valid = get_param_string(design_note, param);
    if(valid) {
        int store = default_value;

        valid = is_modified_time(param, store);
        if(valid) {
            // If we have a valid modified time value, we want to store it 'as is' in the
            // QVarEquation - we can do that by passing the equation an empty string and the
            // value to store as a default
            return DesignParamInt::init("", store, add_listeners);
        } else {
            // Otherwise, this might be a qvar calc
            return DesignParamInt::init(param, store, add_listeners);
        }
    }

    return false;
}


/* ------------------------------------------------------------------------
 *  DesignParamBool
 */

namespace {
    /** Try to parse the specified param string as a boolean.
     *  A boolean is either 't', 'f', 'y', 'n', or either a
     *  number of a qvar string.
     *
     * @param param A string containing the bool string to parse
     * @param store A reference to a bool to store the result in
     * @return true if the string contains a recognised bool string,
     *         false if it appears not to (either it contains nothing
     *         the function can pars,e or it appears to contain more
     *         characters than a bool string should).
     */
    bool is_boolean(const std::string& param, bool& store)
    {
        const char first = tolower(param[0]);

        if(first == 't' || first == 'y') {
            store = true;
        } else if(first == 'f' || first == 'n') {
            store = false;
        } else {
            return false;
        }

        return true;
    }
}


bool DesignParamBool::init(const std::string& design_note, bool default_value, const bool add_listeners)
{
    std::string param;

    // Fetch the raw string from the design note
    bool valid = get_param_string(design_note, param);
    if(valid) {
        bool store = default_value;

        valid = is_boolean(param, store);
        if(valid) {
            // If we have a valid modified time value, we want to store it 'as is' in the
            // QVarEquation - we can do that by passing the equation an empty string and the
            // value to store as a default
            return DesignParamInt::init("", store ? 1 : 0, add_listeners);
        } else {
            // Otherwise, this might be a qvar calc
            return DesignParamInt::init(param, store ? 1 : 0, add_listeners);
        }
    }

    return false;
}


/* ------------------------------------------------------------------------
 *  DesignParamTarget
 */


namespace {
    /** Determine whether the specified parameter contains a target
     *  string that can not have its results cached. This inspects
     *  the provided parameter and returns true if it contains a
     *  target string  that may match different objects each time
     *  value() is called.
     *
     * @param param A reference to a string containing the parameter to check.
     * @return true if the parameter string contains an uncacheable
     *         target string, false otherwise.
     */
    bool is_complex_target(const std::string& param)
    {
        char sigil = param[0];

        // Complex targets include all forms of target string that may need
        // to be fully processed with each call to value() - they are generally
        // easy to identify as they either contain [source] or start with a
        // recognised sigils
        return (param == "[source]" ||          // message sender
                sigil == '&' ||                 // link search
                sigil == '*' || sigil == '@' || // archetype search
                sigil == '<' || sigil == '>');  // Radius search
    }
}


bool DesignParamTarget::init(const std::string& design_note, const bool add_listeners)
{
    std::string param;

    // Fetch the raw string from the design note
    bool valid = get_param_string(design_note, param);
    if(valid) {
        // [me] is always going to be the host object id
        if(param == "[me]") {
            objid_cache = host;
            mode = TARGET_INT;

        // Target may be a QVar calculation (leading $ or all digits)
        } else if(qvar_calc.init(param)) {
            mode = TARGET_QVAR;

        // Check for known target incantations
        } else if(is_complex_target(param)) {
            targetstr = param;
            mode = TARGET_COMPLEX;

        // Treat anything else as a bare object name
        } else {
            SInterface<IObjectSystem> ObjectSys(g_pScriptManager);

            objid_cache = ObjectSys -> GetObjectNamed(target);
            mode = TARGET_INT;
        }
    }

    return false;
}







/* ------------------------------------------------------------------------
 *  Targetting
 */

std::vector<TargetObj>* DesignParamTarget::get_target_objects(const char* target, sScrMsg* msg)
{
    std::vector<TargetObj>* matches = new std::vector<TargetObj>;

    // Make sure target is actually set before doing anything
    if(!target || *target == '\0') return matches;

    float radius;
    bool  lessthan;
    const char* archname;
    TargetObj newtarget = { 0, 0 };

    // Simple message source selection.
    if(!_stricmp(target, "[source]")) {
        newtarget.obj_id = msg -> from;
        matches -> push_back(newtarget);

    // objects linked to the host object through a named link
    } else if(*target == '&') {
        link_search(matches, host, &target[1]);

    // Archetype search, direct concrete and indirect concrete
    } else if(*target == '*' || *target == '@') {
        archetype_search(matches, &target[1], *target == '@');

    // Radius archetype search
    } else if(*target == '<' || *target == '>') {
        if(radius_search(target, &radius, &lessthan, &archname)) {
            const char* realname = archname;
            // Jump filter controls if needed...
            if(*archname == '*' || *archname == '@') ++realname;

            // Default behaviour for radius search is to get all decendants unless * is specified.
            archetype_search(matches, realname, *archname != '*', true, msg -> to, radius, lessthan);
        }

    // Give up, treat as named destination object
    } else {
    }

    return matches;
}


/* ------------------------------------------------------------------------
 *  Link Targetting
 */

void DesignParamTarget::link_search(std::vector<TargetObj>* matches, const int from, const char* linkdef)
{
    std::vector<LinkScanWorker> links;
    bool is_random = false, is_weighted = false, fetch_all = false;;
    uint fetch_count = 0;
    LinkMode mode = LM_BOTH;

    // Parse the link definition, and fetch the list of possible matching links
    const char* flavour = link_search_setup(linkdef, &is_random, &is_weighted, &fetch_count, &fetch_all, &mode);
    uint count = link_scan(flavour, from, is_weighted, mode, links);

    if(count) {
        // If no fetch count has been explicitly set, use the whole size, unless random is set
        if(fetch_count < 1) fetch_count = is_random ? 1 : links.size();

        // if fetch_all has been set, set the count to the link count even in random mode
        if(fetch_all) fetch_count = links.size();

        if(is_random) {
            select_random_links(matches, links, fetch_count, fetch_all, count, is_weighted);
        } else {
            select_links(matches, links, fetch_count);
        }
    }
}


const char* DesignParamTarget::link_search_setup(const char* linkdef, bool* is_random, bool* is_weighted, uint* fetch_count, bool *fetch_all, LinkMode *mode)
{
    while(*linkdef) {
        switch(*linkdef) {
            // The ? sigil indicates that the link mode should be random
            case '?': *is_random = true;
                break;

            // The ! sigil indicates that all links should be returned
            case '!': *fetch_all = true;
                break;

            // [ indicates the start of a [N] block, probably
            case '[': linkdef = parse_link_count(linkdef, fetch_count);

                // If the linkdef char is still [, what follows is not a number,
                // the ++linkdef below will skip the [.
                break;

            case '%': *mode = LM_ARCHETYPE;
                break;

            case '#': *mode = LM_CONCRETE;
                break;

            // Not a recognised sigil? Assume that it's the start of a link flavour
            // name (or "Weighted", in which case enabled weighted random mode)
            default: if(!strcasecmp(linkdef, "Weighted")) {
                        *is_weighted = *is_random = true;
                        *fetch_all = false; // Can't use fetch_all mode for weighted random
                        return "ScriptParams"; // Weighted mode looks at scriptparams
                     }
                     return linkdef;
                break;
        }
        ++linkdef;
    }

    // Fallback for a horribly broken string is always "ControlDevice"
    return "ControlDevice";
}


const char* DesignParamTarget::parse_link_count(const char* linkdef, uint* fetch_count)
{
    // linkdef should be a pointer to a '[' - check to be sure
    if(*linkdef == '[') {
        ++linkdef;

        // Is the link count a qvar?
        // Should we just pull everything between [ and ] and work on it?

        char* endptr;
        int value = strtol(linkdef, &endptr, 10);

        // Has anything been parsed at all?
        if(endptr != linkdef) {
            // A value was parsed, but only positive non-zero values make any sense
            if(value > 0) {
                *fetch_count = value;
            }

            // copy the end pointer so that we can try to find the ]
            const char *close = endptr;

            // Skip anything up to the ] if possible
            while(*close && *close != ']')
                ++close;

            // If the close ] was found, return the pointer to it. If it wasn't,
            // return the pointer to the last character in the number, as link_search_setup
            // will immediately ++ this on return.
            return *close ? close : --endptr;
        }

        // The data after the [ was not numeric, so return the pointer to the [
        // so that link_search_setup can skip it.
        return --linkdef;
    }

    // Not a number block, why was this even called?
    return linkdef;
}


uint DesignParamTarget::link_scan(const char* flavour, const int from, const bool weighted, LinkMode mode, std::vector<LinkScanWorker>& links)
{
    // If there is no link flavour, do nothing
    if(!flavour || !*flavour) return 0;

    SService<ILinkToolsSrv> LinkToolsSrv(g_pScriptManager);

    uint accumulator = 0;
    long flavourid =  LinkToolsSrv -> LinkKindNamed(flavour);

    if(flavourid) {
        // At this point, we need to locate all the linked objects that match the flavour and mode
        SService<ILinkSrv> LinkSrv(g_pScriptManager);
        linkset matching_links;
        LinkScanWorker temp = { 0, 0, 0, 0 };

        // Traverse the list of links that match the selected flavour.
        LinkSrv -> GetAll(matching_links, flavourid, from, 0);
        while(matching_links.AnyLinksLeft()) {
            // Get the common link information
            temp.weight  = 1;
            temp.link_id = matching_links.Link();
            temp.dest_id = matching_links.Get().dest;

            if(mode == LM_BOTH ||                            // If linkmode is both, let through any destination
               (temp.dest_id < 0 && mode == LM_ARCHETYPE) || // Otherwisse, only let through archetypes or concrete if set
               (temp.dest_id > 0 && mode == LM_CONCRETE)) {

                // If weighting is enabled, fetch the weighting information from the link.
                if(weighted) {
                    const char* data = static_cast<const char* >(matching_links.Data());

                    temp.weight = strtol(data, NULL, 10);
                    if(temp.weight < 1) temp.weight = 1; // Force positive non-zero weights
                    accumulator += temp.weight;
                }

                links.push_back(temp);
            }

            matching_links.NextLink();
        }
    }

    // Ensure that the list is sorted by link IDs. In theory it already should be, but
    // this will guarantee it.
    if(links.size() > 0)
        std::sort(links.begin(), links.end());

    if(weighted) {
        return accumulator;
    } else {
        return links.size();
    }
}


bool DesignParamTarget::pick_weighted_link(std::vector<LinkScanWorker>& links, const uint target, TargetObj& store)
{
    std::vector<LinkScanWorker>::iterator it;

    for(it = links.begin(); it < links.end(); it++) {
        if((*it).cumulative >= target) {
            store = *it;
            return true;
        }
    }

    return false;
}


uint DesignParamTarget::build_link_weightsums(std::vector<LinkScanWorker>& links)
{
    uint accumulator = 0;
    std::vector<LinkScanWorker>::iterator it;

    for(it = links.begin(); it < links.end(); it++) {
        accumulator += it -> weight;
        it -> cumulative = accumulator;
    }

    return accumulator;
}


void DesignParamTarget::select_random_links(std::vector<TargetObj>* matches, std::vector<LinkScanWorker>& links, const uint fetch_count, const bool fetch_all, const uint total_weights, const bool is_weighted)
{
    // Yay for easy randomisation
    std::shuffle(links.begin(), links.end(), randomiser);

    if(!is_weighted) {
        // Work out how many links to fetch, limiting it to the number available.
        uint count = fetch_all ? links.size() : fetch_count;
        if(count > links.size()) count = links.size();

        select_links(matches, links, count);

    } else {
        // Weighted selection needs cumulative weight information
        if(is_weighted) build_link_weightsums(links);

        TargetObj chosen;

        // Pick the requested number of links
        for(uint pass = 0; pass < fetch_count; ++pass) {
            // Weighted mode needs more work to pick the item
            pick_weighted_link(links, 1 + (randomiser() % total_weights), chosen);

            // Store the chosen item
            matches -> push_back(chosen);
        }
    }
}


void DesignParamTarget::select_links(std::vector<TargetObj>* matches, std::vector<LinkScanWorker>& links, const uint fetch_count)
{
    uint copied = 0;
    TargetObj newtemp = { 0, 0 };
    std::vector<LinkScanWorker>::iterator it;

    for(it = links.begin(); it < links.end() && copied < fetch_count; it++, copied++) {
        newtemp = *it;
        matches -> push_back(newtemp);
    }
}


/* ------------------------------------------------------------------------
 *  Search methods
 */

bool DesignParamTarget::radius_search(const char* target, float* radius, bool* lessthan, const char** archetype)
{
    // Check for < or > here
    *lessthan = (*target++ == '<');

    // try to parse the radius
    char* end;
    *radius = strtof(target, &end);

    if(!end || end == target) return false;

    // Look for the ':' in the string
    while(*end && *end != ':') ++end;

    // Hit end of string without finding a ':'? Give up.
    if(!*end) return false;

    // Archetype starts right after the ':'
    *archetype = ++end;

    // Make sure that end of string after ':' doesn't bite us.
    if(!**archetype) return false;

    // Okay, this should be a radius search!
    return true;
}


void DesignParamTarget::archetype_search(std::vector<TargetObj>* matches, const char* archetype, bool do_full, bool do_radius, object from_obj, float radius, bool lessthan)
{
    // Get handles to game interfaces here for convenience
    SInterface<IObjectSystem> ObjectSys(g_pScriptManager);
	SService<IObjectSrv>      ObjectSrv(g_pScriptManager);
    SInterface<ITraitManager> TraitMgr(g_pScriptManager);

    // These are only needed when doing radius searches
    cScrVec from_pos, to_pos;
    float   distance;
    if(do_radius) ObjectSrv -> Position(from_pos, from_obj);

    // Find the archetype named if possible
    object arch = ObjectSys -> GetObjectNamed(archetype);
    if(int(arch) <= 0) {

        // Build the query flags
        ulong flags = kTraitQueryChildren;
        if(do_full) flags |= kTraitQueryFull; // If dofull is on, query direct and indirect descendants

        // Ask for the list of matching objects
        SInterface<IObjectQuery> query = TraitMgr -> Query(arch, flags);
        if(query) {
            TargetObj newtarget = { 0, 0 };

            // Process each object, adding it to the match list if it's concrete.
            for(; !query -> Done(); query -> Next()) {
                newtarget.obj_id = query -> Object();
                if(newtarget.obj_id > 0) {

                    // Object is concrete, do we need to check it for distance?
                    if(do_radius) {
                        // Get the provisionally matched object's position, and work out how far it
                        // is from the 'from' object.
                        ObjectSrv -> Position(to_pos, newtarget.obj_id);
                        distance = (float)from_pos.Distance(to_pos);

                        // If the distance check passes, store the object.
                        if((lessthan && (distance < radius)) || (!lessthan && (distance > radius))) {
                            matches -> push_back(newtarget);
                        }

                    // No radius check needed, add straight to the list
                    } else {
                        matches -> push_back(newtarget);
                    }
                }
            }
        }
    }
}

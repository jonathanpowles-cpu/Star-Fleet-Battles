#include "board.h"
#include "ship.h"
#include "renderer.h"
#include <SDL.h>
#include <SDL_ttf.h>
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include <commdlg.h>
#include <SDL_syswm.h>
#include <deque>
#include <vector>
#include <string>
#include <map>
#include <array>
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <sstream>
#include <algorithm>
#include <cmath>

enum class GamePhase { Setup, TerrainSetup, Placement, Planning, Impulse, GameOver };

// â”€â”€ Setup entries (all available ships) â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€
struct SetupEntry {
    std::string    name;
    std::string    ship_class; // key for build_ship dispatch
    Faction        faction;
    bool           included   = true;
    ShipController controller = ShipController::AI;
    int            class_idx  = 0; // index into FACTION_CLASSES[faction]
};

// Available classes per faction: {key, display_label}
using ClassList = std::vector<std::pair<std::string,std::string>>;
static const ClassList FEDERATION_CLASSES = {
    {"fed_ca","CA"},{"fed_ff","FF"},{"fed_dd","DD"},{"fed_sc","SC"},
    {"fed_cl","CL"},{"fed_cc","CC"},{"fed_dn","DN"},{"axcvl","CVL"},
    // New from MSB/C1/R-series
    {"fed_cvb","CVB"},
    {"fed_dwm","DWM"},
    {"fed_bcf","BCF"},
    {"andro_ana","ANA"},
    {"fed_fft","FFT"},
    {"fed_ffm","FFM"},
    {"fed_cve","CVE"},
    {"gen_axcvl","AXCVL"},
    {"fed_ddl","DDL"},
    {"fed_cs","CS"},
    {"fed_cov","COV"},
    {"andro_asp","ASP"},
    {"fed_ffp","FFP"},
    {"fed_cvl","CVL"},
    {"fed_ddg","DDG"},
    {"fed_cx","CX"},
    {"fed_nsc","NSC"},
    {"fed_cad","CAD"},
    {"fed_cva","CVA"},
    {"fed_pv","PV"},
    {"fed_ncc","NCC"},
    {"fed_bcg","BCG"},
    {"fed_cvs","CVS"},
    {"fed_lbt","LBT"},
    {"fed_dwa","DWA"},
    {"fed_ca_plus","CA+"},
    {"fed_hcc","HCC"},
    {"fed_ffl","FFL"},
    {"fed_nvl","NVL"},
    {"fed_ffd","FFD"},
    {"fed_dn_plus","DN+"},
    {"fed_nvh","NVH"},
    {"andro_kin","KIN"},
    {"fed_car","CAR"},
    {"fed_scs","SCS"},
    {"fed_dwc","DWC"},
    {"fed_bcs","BCS"},
    {"fed_cb","CB"},
    {"fed_dwt","DWT"},
    {"fed_cmc_cov","COV"},
    {"fed_nvs","NVS"},
    {"fed_gsc","GSC"},
    {"fed_cls","CLS"},
    {"fed_ffg","FFG"},
    {"fed_ner","NER"},
    {"fed_ecl","ECL"},
    {"fed_dwe","DWE"},
    {"fed_ffv","FFV"},
    {"fed_de","DE"},
    {"fed_scout_dd","SC"},
    {"fed_cf","CF"},
    {"fed_dnl","DNL"},
    {"fed_scsa","SCSA"},
    {"fed_clh","CLH"},
    {"fed_ffs","FFS"},
    {"fed_ffa","FFA"},
    {"gen_axcva","AXCVA"},
    {"fed_der","DER"},
    {"fed_ncva","NCVA"},
    {"fed_fra","FRA"},
    {"fed_nec","NEC"},
    {"fed_dwd","DWD"},
    {"fed_dar","DAR"},
    {"fed_dea","DEA"},
    {"fed_nac","NAC"},
    {"fed_ncl","NCL"},
    {"fed_ffe","FFE"},
    {"fed_nca","NCA"},
    {"fed_ffb","FFB"},
    {"fed_ncd","NCD"},
    {"fed_dw","DW"},
    {"fed_ltt","LTT"},
    {"fed_acl","ACL"},
    {"andro_kra","KRA"},
    {"fed_ffr","FFR"},
    {"fed_dws","DWS"},
    {"fed_nhcc","NHCC"},
    {"fed_cmc","CMC"},
    {"fed_bb","BB"},
    {"fed_bcj","BCJ"},
    {"fed_clc","CLC"},
    {"fed_nea","NEA"},
    {"fed_dng","DNG"}
};
static const ClassList KLINGON_CLASSES = {
    {"klingon_d7","D7"},{"klingon_d6","D6"},{"klingon_c8","C8"},
    {"klingon_c9","C9"},{"klingon_f5","F5"},{"klingon_e4","E4"},
    // New from MSB/R-series
    {"klingon_d5g","D5G"},
    {"klingon_d7d","D7D"},
    {"klingon_f5x","FX"},
    {"klingon_d5i","D5I"},
    {"klingon_d5h","D5H"},
    {"kli_c8v","C8V"},
    {"klingon_d5v","D5V"},
    {"klingon_f5j","F5J"},
    {"klingon_d6j","D6J"},
    {"klingon_f6","F6"},
    {"klingon_d6e","D6E"},
    {"klingon_md5","MD5"},
    {"klingon_d5f","D5F"},
    {"klingon_c7","C7"},
    {"klingon_c8s","C8S"},
    {"klingon_d5k","D5K"},
    {"kli_ad5","AD5"},
    {"klingon_d7m","D7M"},
    {"kli_e4e","E4E"},
    {"klingon_d7v","D7V"},
    {"klingon_af5","AF5"},
    {"klingon_e4d","E4D"},
    {"klingon_e4v","E4V"},
    {"klingon_d5j","D5J"},
    {"klingon_e5","E5"},
    {"klingon_d6g","D6G"},
    {"klingon_c7a","C7A"},
    {"klingon_e3d","E3D"},
    {"klingon_e4j","E4J"},
    {"klingon_d7e","D7E"},
    {"kli_d7v","D7V"},
    {"klingon_f5e","F5E"},
    {"kli_f5v","F5V"},
    {"klingon_ad6","AD6"},
    {"klingon_d5d","D5D"},
    {"klingon_d7cx","DX"},
    {"klingon_d7n","D7N"},
    {"klingon_d5s","D5S"},
    {"klingon_d5m","D5M"},
    {"klingon_d5c","D5C"},
    {"klingon_d5p","D5P"},
    {"klingon_c9a","C9A"},
    {"klingon_d5n","D5N"},
    {"klingon_d5e","D5E"},
    {"klingon_d5l","D5L"},
    {"klingon_rkl","RKL"},
    {"klingon_d6s","D6S"}
};
static const ClassList ROMULAN_CLASSES = {
    {"romulan_kr","KR"},{"romulan_condor","DN"},{"romulan_ke","KE"},
    {"romulan_we","WE"},{"romulan_sh","CL"},{"romulan_sky","DD"},
    {"romulan_snipe","FF"},{"romulan_wb","WB"},{"romulan_k5r","K5R"},
};
static const ClassList GORN_CLASSES = {
    {"gorn_ca","CA"},{"gorn_bc","BC"},{"gorn_cl","CL"},
    {"gorn_dd","DD"},{"gorn_ddf","DDF"},{"gorn_sc","SC"},{"gorn_ff","FF"},
};
static const ClassList KZINTI_CLASSES = {
    {"kzinti_cw","CW"},{"kzinti_bc","BC"},{"kzinti_cc","CC"},
    {"kzinti_cl","CL"},{"kzinti_cv","CV"},{"kzinti_cvs","CVS"},{"kzinti_ff","FF"},
    // New from R-series
    {"kzin_ca","CA"},
    {"kzin_ddv","DDV"},
    {"kzin_dws","DWS"},
    {"kzin_dwa","DWA"},
    {"kzin_mec","MEC"},
    {"kzin_cva","CVA"},
    {"kzin_pol","POL"},
    {"kzin_srv","SRV"},
    {"kzin_dwd","DWD"},
    {"kzin_mpf","MPF"},
    {"kzin_dd","DD"},
    {"kzin_sdf","SDF"},
    {"kzin_cd","CD"},
    {"kzin_sr","SR"},
    {"kzin_mdc","MDC"},
    {"kzin_mcc","MCC"},
    {"kzin_dw","DW"},
    {"kzin_mcv","MCV"},
    {"kzin_dwe","DWE"},
    {"kzin_mac","MAC"},
    {"kzin_dn","DN"},
    {"kzin_mms","MMS"},
    {"kzin_ffk","FFK"},
    {"kzin_bch","BCH"},
    {"kzin_fh","FH"},
    {"kzin_sscs","SSCS"},
    {"kzin_msc","MSC"},
    {"kzin_dwl","DWL"},
    {"kzin_mtt","MTT"}
};
static const ClassList THOLIAN_CLASSES = {
    {"tholian_pc","PC"},{"tholian_pc_plus","PC+"},
    {"tholian_dd","DD"},{"tholian_co","CO"},
};
static const ClassList ORION_CLASSES = {
    {"orion_cr","CR"},{"orion_lr","LR"},{"orion_br","BR"},{"orion_ca","CA"},
    // New from R-series
    {"orion_cvs","CVS"},
    {"orion_brp","BRP"},
    {"orion_hr","HR"},
    {"orion_dbr","DBR"},
    {"orion_bch","BCH"},
    {"orion_dw","DW"},
    {"orion_lrs","LRS"},
    {"orion_ar","AR"},
    {"orion_dws","DWS"},
    {"orion_dbp","DBP"},
    {"orion_mr","MR"},
    {"orion_ok6","OK6"}
};
static const ClassList HYDRAN_CLASSES = {
    {"hydran_hdn","DN"},{"hydran_lord","CC"},
    {"hydran_ranger","CL"},{"hydran_lancer","DD"},{"hydran_cuirassier","FF"},
    // New from C1
    {"hydran_apa","APA"},
    {"hydran_nvl","NVL"},
    {"hydran_dg","DG"},
    {"hydran_mng","MNG"},
    {"hydran_pal","PAL"},
    {"hydran_id","ID"},
    {"hydran_ms","MS"},
    {"hydran_cru","CRU"},
    {"hydran_bar","BAR"},
    {"hydran_sc","SC"},
    {"hydran_hr","HR"},
    {"hydran_ov","OV"},
    {"hydran_sar","SAR"},
    {"hydran_lb","LB"},
    {"hydran_hn","HN"},
    {"hydran_tar","TAR"},
    {"hydran_nsc","NSC"},
    {"hydran_cos","COS"},
    {"hydran_com","COM"},
    {"hydran_cve","CVE"},
    {"hydran_tr","TR"},
    {"hydran_npf","NPF"},
    {"hydran_nac","NAC"},
    {"hydran_srv","SRV"},
    {"hydran_cu","CU"},
    {"hydran_cat","CAT"},
    {"hydran_lp","LP"},
    {"hydran_rn","RN"},
    {"hydran_uh","UH"},
    {"hydran_srg","SRG"},
    {"hydran_kn","KN"},
    {"hydran_gen","GEN"},
    {"hydran_lc","LC"},
    {"hydran_ltt","LTT"},
    {"hydran_da","DA"},
    {"hydran_eh","EH"},
    {"hydran_lm","LM"},
    {"hydran_ln","LN"},
    {"hydran_d7h","D7H"},
    {"hydran_war","WAR"},
    {"hydran_cav","CAV"},
    {"hydran_nms","NMS"},
    {"hydran_nec","NEC"},
    {"hydran_de","DE"},
    {"hydran_sr","SR"}
};
static const ClassList LYRAN_CLASSES = {
    {"lyran_ca","CA"},{"lyran_bc","BC"},
    {"lyran_cl","CL"},{"lyran_dd","DD"},{"lyran_ff","FF"},
    // New from C1 + WYN cluster
    {"lyran_tgc","TGC"},
    {"wyn_obr","OBR"},
    {"lyran_dwl","DWL"},
    {"lyran_ffe","FFE"},
    {"wyn_ldd","Lyran"},
    {"lyran_scs","SCS"},
    {"lyran_cv","CV"},
    {"lyran_tgp","TGP"},
    {"wyn_olr","OLR"},
    {"wyn_kg2","Klingon"},
    {"lyran_cwa","CWA"},
    {"lyran_dwm","DWM"},
    {"lyran_dw","DW"},
    {"wyn_axms","AxMS"},
    {"lyran_bch","BCH"},
    {"lyran_dn","DN"},
    {"wyn_odr","ODR"},
    {"wyn_axbc","AxBC"},
    {"lyran_mp","MP"},
    {"wyn_pbb","PBB"},
    {"lyran_cvl","CVL"},
    {"lyran_ltt","LTT"},
    {"lyran_pol","POL"},
    {"lyran_dwa","DWA"},
    {"lyran_stt","STT"},
    {"lyran_ms","MS"},
    {"lyran_cwl","CWL"},
    {"wyn_ke4","KE4"},
    {"lyran_sr","SR"},
    {"wyn_axc","AxC"},
    {"wyn_axcv","AxCV"},
    {"lyran_dwe","DWE"},
    {"lyran_cwe","CWE"},
    {"lyran_pfw","PFW"},
    {"lyran_dws","DWS"},
    {"lyran_stj","STJ"},
    {"lyran_srv","SRV"},
    {"wyn_axcva","AxCVA"},
    {"lyran_cc","CC"},
    {"lyran_cw","CW"},
    {"lyran_ltv","LTV"},
    {"lyran_ffa","FFA"},
    {"wyn_ocr","OCR"},
    {"lyran_sc","SC"},
    {"lyran_cwm","CWM"},
    {"lyran_cws","CWS"},
    {"wyn_zff","Kzinti"}
};

static const ClassList& faction_classes(Faction f) {
    switch (f) {
        case Faction::Federation: return FEDERATION_CLASSES;
        case Faction::Klingon:    return KLINGON_CLASSES;
        case Faction::Romulan:    return ROMULAN_CLASSES;
        case Faction::Gorn:       return GORN_CLASSES;
        case Faction::Kzinti:     return KZINTI_CLASSES;
        case Faction::Tholian:    return THOLIAN_CLASSES;
        case Faction::Orion:      return ORION_CLASSES;
        case Faction::Hydran:     return HYDRAN_CLASSES;
        case Faction::Lyran:      return LYRAN_CLASSES;
    }
    return FEDERATION_CLASSES;
}

static std::vector<SetupEntry> default_setup() {
    return {
        {"Enterprise",     "fed_ca",      Faction::Federation, true,  ShipController::Player1, 0},
        {"IKS Klothos",    "klingon_d7",  Faction::Klingon,    true,  ShipController::AI,      0},
        {"RIS Serapis",    "romulan_kr",  Faction::Romulan,    false, ShipController::AI,      0},
        {"GCS Ghestur",    "gorn_ca",     Faction::Gorn,       false, ShipController::AI,      0},
        {"KCSS Kzar",      "kzinti_cw",   Faction::Kzinti,     false, ShipController::AI,      0},
        {"TAS Tarantula",  "tholian_pc",  Faction::Tholian,    false, ShipController::AI,      0},
        {"OPV Syndicate",  "orion_cr",    Faction::Orion,      false, ShipController::AI,      0},
        {"HKS Monarch",    "hydran_hdn",  Faction::Hydran,     false, ShipController::AI,      0},
        {"LSS Inquisitor", "lyran_ca",    Faction::Lyran,      false, ShipController::AI,      0},
    };
}

// Starting positions / facings for up to 5 ships (spread around the board)
static const Hex   START_POS[5]    = {{0,-5},{-4,5},{4,4},{-4,-1},{4,-5}};
static const int   START_FACING[5] = {3, 1, 5, 2, 4};

using ShipFactory = Ship(*)(std::string, Hex, int);
static const std::pair<const char*, ShipFactory> SHIP_TABLE[] = {
    {"fed_ca", make_federation_ca},
    {"fed_ff", make_federation_ff},
    {"fed_dd", make_federation_dd},
    {"fed_sc", make_federation_sc},
    {"fed_cl", make_federation_cl},
    {"fed_cc", make_federation_cc},
    {"fed_dn", make_federation_dn},
    {"axcvl", make_axcvl},
    {"klingon_d7", make_klingon_d7},
    {"klingon_d6", make_klingon_d6},
    {"klingon_c8", make_klingon_c8},
    {"klingon_c9", make_klingon_c9},
    {"klingon_f5", make_klingon_f5},
    {"klingon_e4", make_klingon_e4},
    {"romulan_kr", make_romulan_kr},
    {"romulan_condor", make_romulan_condor},
    {"romulan_ke", make_romulan_ke},
    {"romulan_we", make_romulan_we},
    {"romulan_sh", make_romulan_sh},
    {"romulan_sky", make_romulan_sky},
    {"romulan_snipe", make_romulan_snipe},
    {"romulan_wb", make_romulan_wb},
    {"romulan_k5r", make_romulan_k5r},
    {"gorn_ca", make_gorn_ca},
    {"gorn_bc", make_gorn_bc},
    {"gorn_cl", make_gorn_cl},
    {"gorn_dd", make_gorn_dd},
    {"gorn_ddf", make_gorn_ddf},
    {"gorn_sc", make_gorn_sc},
    {"gorn_ff", make_gorn_ff},
    {"kzinti_cw", make_kzinti_cw},
    {"kzinti_bc", make_kzinti_bc},
    {"kzinti_cc", make_kzinti_cc},
    {"kzinti_cl", make_kzinti_cl},
    {"kzinti_cv", make_kzinti_cv},
    {"kzinti_cvs", make_kzinti_cvs},
    {"kzinti_ff", make_kzinti_ff},
    {"tholian_pc", make_tholian_pc},
    {"tholian_pc_plus", make_tholian_pc_plus},
    {"tholian_dd", make_tholian_dd},
    {"tholian_co", make_tholian_co},
    {"orion_cr", make_orion_cr},
    {"orion_lr", make_orion_lr},
    {"orion_br", make_orion_br},
    {"orion_ca", make_orion_ca},
    {"hydran_hdn", make_hydran_hdn},
    {"hydran_lord", make_hydran_lord},
    {"hydran_ranger", make_hydran_ranger},
    {"hydran_lancer", make_hydran_lancer},
    {"hydran_cuirassier", make_hydran_cuirassier},
    {"lyran_ca", make_lyran_ca},
    {"lyran_bc", make_lyran_bc},
    {"lyran_cl", make_lyran_cl},
    {"lyran_dd", make_lyran_dd},
    {"lyran_ff", make_lyran_ff},
    {"hydran_apa", make_hydran_apa},
    {"fed_cvb", make_fed_cvb},
    {"fed_dwm", make_fed_dwm},
    {"kzin_ca", make_kzin_ca},
    {"hydran_nvl", make_hydran_nvl},
    {"lyran_tgc", make_lyran_tgc},
    {"fed_bcf", make_fed_bcf},
    {"andro_ana", make_andro_ana},
    {"klingon_d5g", make_klingon_d5g},
    {"kzin_ddv", make_kzin_ddv},
    {"fed_fft", make_fed_fft},
    {"kzin_dws", make_kzin_dws},
    {"fed_ffm", make_fed_ffm},
    {"klingon_d7d", make_klingon_d7d},
    {"hydran_dg", make_hydran_dg},
    {"fed_cve", make_fed_cve},
    {"gen_axcvl", make_gen_axcvl},
    {"wyn_obr", make_wyn_obr},
    {"hydran_mng", make_hydran_mng},
    {"orion_cvs", make_orion_cvs},
    {"klingon_f5x", make_klingon_f5x},
    {"klingon_d5i", make_klingon_d5i},
    {"fed_ddl", make_fed_ddl},
    {"fed_cs", make_fed_cs},
    {"orion_brp", make_orion_brp},
    {"klingon_d5h", make_klingon_d5h},
    {"fed_cov", make_fed_cov},
    {"hydran_pal", make_hydran_pal},
    {"kzin_dwa", make_kzin_dwa},
    {"lyran_dwl", make_lyran_dwl},
    {"kli_c8v", make_kli_c8v},
    {"lyran_ffe", make_lyran_ffe},
    {"wyn_ldd", make_wyn_ldd},
    {"lyran_scs", make_lyran_scs},
    {"klingon_d5v", make_klingon_d5v},
    {"hydran_id", make_hydran_id},
    {"hydran_ms", make_hydran_ms},
    {"lyran_cv", make_lyran_cv},
    {"andro_asp", make_andro_asp},
    {"fed_ffp", make_fed_ffp},
    {"fed_cvl", make_fed_cvl},
    {"klingon_f5j", make_klingon_f5j},
    {"fed_ddg", make_fed_ddg},
    {"lyran_tgp", make_lyran_tgp},
    {"wyn_olr", make_wyn_olr},
    {"fed_cx", make_fed_cx},
    {"hydran_cru", make_hydran_cru},
    {"kzin_mec", make_kzin_mec},
    {"hydran_bar", make_hydran_bar},
    {"wyn_kg2", make_wyn_kg2},
    {"klingon_d6j", make_klingon_d6j},
    {"fed_nsc", make_fed_nsc},
    {"klingon_f6", make_klingon_f6},
    {"kzin_cva", make_kzin_cva},
    {"hydran_sc", make_hydran_sc},
    {"fed_cad", make_fed_cad},
    {"kzin_pol", make_kzin_pol},
    {"fed_cva", make_fed_cva},
    {"klingon_d6e", make_klingon_d6e},
    {"hydran_hr", make_hydran_hr},
    {"fed_pv", make_fed_pv},
    {"orion_hr", make_orion_hr},
    {"kzin_srv", make_kzin_srv},
    {"lyran_cwa", make_lyran_cwa},
    {"klingon_md5", make_klingon_md5},
    {"fed_ncc", make_fed_ncc},
    {"kzin_dwd", make_kzin_dwd},
    {"kzin_mpf", make_kzin_mpf},
    {"kzin_dd", make_kzin_dd},
    {"fed_bcg", make_fed_bcg},
    {"klingon_d5f", make_klingon_d5f},
    {"hydran_ov", make_hydran_ov},
    {"kzin_sdf", make_kzin_sdf},
    {"klingon_c7", make_klingon_c7},
    {"hydran_sar", make_hydran_sar},
    {"klingon_c8s", make_klingon_c8s},
    {"klingon_d5k", make_klingon_d5k},
    {"kzin_cd", make_kzin_cd},
    {"fed_cvs", make_fed_cvs},
    {"lyran_dwm", make_lyran_dwm},
    {"kli_ad5", make_kli_ad5},
    {"kzin_sr", make_kzin_sr},
    {"kzin_mdc", make_kzin_mdc},
    {"orion_dbr", make_orion_dbr},
    {"lyran_dw", make_lyran_dw},
    {"kzin_mcc", make_kzin_mcc},
    {"orion_bch", make_orion_bch},
    {"hydran_lb", make_hydran_lb},
    {"hydran_hn", make_hydran_hn},
    {"wyn_axms", make_wyn_axms},
    {"klingon_d7m", make_klingon_d7m},
    {"fed_lbt", make_fed_lbt},
    {"fed_dwa", make_fed_dwa},
    {"fed_ca_plus", make_fed_ca_plus},
    {"fed_hcc", make_fed_hcc},
    {"lyran_bch", make_lyran_bch},
    {"hydran_tar", make_hydran_tar},
    {"hydran_nsc", make_hydran_nsc},
    {"lyran_dn", make_lyran_dn},
    {"hydran_cos", make_hydran_cos},
    {"fed_ffl", make_fed_ffl},
    {"wyn_odr", make_wyn_odr},
    {"kli_e4e", make_kli_e4e},
    {"hydran_com", make_hydran_com},
    {"fed_nvl", make_fed_nvl},
    {"fed_ffd", make_fed_ffd},
    {"fed_dn_plus", make_fed_dn_plus},
    {"fed_nvh", make_fed_nvh},
    {"klingon_d7v", make_klingon_d7v},
    {"kzin_dw", make_kzin_dw},
    {"andro_kin", make_andro_kin},
    {"wyn_axbc", make_wyn_axbc},
    {"lyran_mp", make_lyran_mp},
    {"wyn_pbb", make_wyn_pbb},
    {"fed_car", make_fed_car},
    {"fed_scs", make_fed_scs},
    {"fed_dwc", make_fed_dwc},
    {"fed_bcs", make_fed_bcs},
    {"hydran_cve", make_hydran_cve},
    {"fed_cb", make_fed_cb},
    {"fed_dwt", make_fed_dwt},
    {"orion_dw", make_orion_dw},
    {"hydran_tr", make_hydran_tr},
    {"hydran_npf", make_hydran_npf},
    {"fed_cmc_cov", make_fed_cmc_cov},
    {"hydran_nac", make_hydran_nac},
    {"fed_nvs", make_fed_nvs},
    {"lyran_cvl", make_lyran_cvl},
    {"lyran_ltt", make_lyran_ltt},
    {"klingon_af5", make_klingon_af5},
    {"kzin_mcv", make_kzin_mcv},
    {"hydran_srv", make_hydran_srv},
    {"klingon_e4d", make_klingon_e4d},
    {"fed_gsc", make_fed_gsc},
    {"lyran_pol", make_lyran_pol},
    {"hydran_cu", make_hydran_cu},
    {"fed_cls", make_fed_cls},
    {"lyran_dwa", make_lyran_dwa},
    {"fed_ffg", make_fed_ffg},
    {"orion_lrs", make_orion_lrs},
    {"hydran_cat", make_hydran_cat},
    {"klingon_e4v", make_klingon_e4v},
    {"lyran_stt", make_lyran_stt},
    {"hydran_lp", make_hydran_lp},
    {"fed_ner", make_fed_ner},
    {"lyran_ms", make_lyran_ms},
    {"fed_ecl", make_fed_ecl},
    {"lyran_cwl", make_lyran_cwl},
    {"fed_dwe", make_fed_dwe},
    {"hydran_rn", make_hydran_rn},
    {"klingon_d5j", make_klingon_d5j},
    {"kzin_dwe", make_kzin_dwe},
    {"klingon_e5", make_klingon_e5},
    {"fed_ffv", make_fed_ffv},
    {"wyn_ke4", make_wyn_ke4},
    {"klingon_d6g", make_klingon_d6g},
    {"klingon_c7a", make_klingon_c7a},
    {"lyran_sr", make_lyran_sr},
    {"hydran_uh", make_hydran_uh},
    {"wyn_axc", make_wyn_axc},
    {"fed_de", make_fed_de},
    {"fed_scout_dd", make_fed_scout_dd},
    {"hydran_srg", make_hydran_srg},
    {"fed_cf", make_fed_cf},
    {"kzin_mac", make_kzin_mac},
    {"wyn_axcv", make_wyn_axcv},
    {"fed_dnl", make_fed_dnl},
    {"lyran_dwe", make_lyran_dwe},
    {"klingon_e3d", make_klingon_e3d},
    {"klingon_e4j", make_klingon_e4j},
    {"hydran_kn", make_hydran_kn},
    {"fed_scsa", make_fed_scsa},
    {"kzin_dn", make_kzin_dn},
    {"fed_clh", make_fed_clh},
    {"fed_ffs", make_fed_ffs},
    {"kzin_mms", make_kzin_mms},
    {"fed_ffa", make_fed_ffa},
    {"lyran_cwe", make_lyran_cwe},
    {"klingon_d7e", make_klingon_d7e},
    {"kzin_ffk", make_kzin_ffk},
    {"kli_d7v", make_kli_d7v},
    {"gen_axcva", make_gen_axcva},
    {"fed_der", make_fed_der},
    {"kzin_bch", make_kzin_bch},
    {"fed_ncva", make_fed_ncva},
    {"hydran_gen", make_hydran_gen},
    {"fed_fra", make_fed_fra},
    {"klingon_f5e", make_klingon_f5e},
    {"fed_nec", make_fed_nec},
    {"fed_dwd", make_fed_dwd},
    {"hydran_lc", make_hydran_lc},
    {"fed_dar", make_fed_dar},
    {"fed_dea", make_fed_dea},
    {"hydran_ltt", make_hydran_ltt},
    {"kli_f5v", make_kli_f5v},
    {"kzin_fh", make_kzin_fh},
    {"lyran_pfw", make_lyran_pfw},
    {"klingon_ad6", make_klingon_ad6},
    {"fed_nac", make_fed_nac},
    {"fed_ncl", make_fed_ncl},
    {"fed_ffe", make_fed_ffe},
    {"klingon_d5d", make_klingon_d5d},
    {"kzin_sscs", make_kzin_sscs},
    {"kzin_msc", make_kzin_msc},
    {"lyran_dws", make_lyran_dws},
    {"fed_nca", make_fed_nca},
    {"klingon_d7cx", make_klingon_d7cx},
    {"orion_ar", make_orion_ar},
    {"lyran_stj", make_lyran_stj},
    {"hydran_da", make_hydran_da},
    {"lyran_srv", make_lyran_srv},
    {"klingon_d7n", make_klingon_d7n},
    {"hydran_eh", make_hydran_eh},
    {"fed_ffb", make_fed_ffb},
    {"hydran_lm", make_hydran_lm},
    {"klingon_d5s", make_klingon_d5s},
    {"orion_dws", make_orion_dws},
    {"fed_ncd", make_fed_ncd},
    {"fed_dw", make_fed_dw},
    {"orion_dbp", make_orion_dbp},
    {"fed_ltt", make_fed_ltt},
    {"klingon_d5m", make_klingon_d5m},
    {"hydran_ln", make_hydran_ln},
    {"klingon_d5c", make_klingon_d5c},
    {"orion_mr", make_orion_mr},
    {"wyn_axcva", make_wyn_axcva},
    {"fed_acl", make_fed_acl},
    {"lyran_cc", make_lyran_cc},
    {"hydran_d7h", make_hydran_d7h},
    {"andro_kra", make_andro_kra},
    {"fed_ffr", make_fed_ffr},
    {"klingon_d5p", make_klingon_d5p},
    {"lyran_cw", make_lyran_cw},
    {"hydran_war", make_hydran_war},
    {"fed_dws", make_fed_dws},
    {"fed_nhcc", make_fed_nhcc},
    {"fed_cmc", make_fed_cmc},
    {"hydran_cav", make_hydran_cav},
    {"klingon_c9a", make_klingon_c9a},
    {"hydran_nms", make_hydran_nms},
    {"lyran_ltv", make_lyran_ltv},
    {"fed_bb", make_fed_bb},
    {"fed_bcj", make_fed_bcj},
    {"fed_clc", make_fed_clc},
    {"lyran_ffa", make_lyran_ffa},
    {"wyn_ocr", make_wyn_ocr},
    {"lyran_sc", make_lyran_sc},
    {"lyran_cwm", make_lyran_cwm},
    {"klingon_d5n", make_klingon_d5n},
    {"klingon_d5e", make_klingon_d5e},
    {"klingon_d5l", make_klingon_d5l},
    {"lyran_cws", make_lyran_cws},
    {"hydran_nec", make_hydran_nec},
    {"klingon_rkl", make_klingon_rkl},
    {"orion_ok6", make_orion_ok6},
    {"klingon_d6s", make_klingon_d6s},
    {"kzin_dwl", make_kzin_dwl},
    {"kzin_mtt", make_kzin_mtt},
    {"wyn_zff", make_wyn_zff},
    {"hydran_de", make_hydran_de},
    {"fed_nea", make_fed_nea},
    {"fed_dng", make_fed_dng},
    {"hydran_sr", make_hydran_sr},
};

static Ship build_ship(const SetupEntry& e, int slot) {
    Hex pos = START_POS[slot % 5];
    int fac = START_FACING[slot % 5];
    const std::string& sc = e.ship_class;
    Ship s;
    for (auto& [key, fn] : SHIP_TABLE) {
        if (sc == key) { s = fn(e.name, pos, fac); goto done; }
    }
    // Fallback
    switch (e.faction) {
        case Faction::Federation: s = make_federation_ca(e.name,pos,fac); break;
        case Faction::Klingon:    s = make_klingon_d7(e.name,pos,fac);    break;
        case Faction::Romulan:    s = make_romulan_kr(e.name,pos,fac);    break;
        case Faction::Gorn:       s = make_gorn_ca(e.name,pos,fac);       break;
        case Faction::Kzinti:     s = make_kzinti_cw(e.name,pos,fac);     break;
        case Faction::Tholian:    s = make_tholian_pc(e.name,pos,fac);    break;
        case Faction::Orion:      s = make_orion_cr(e.name,pos,fac);      break;
        case Faction::Hydran:     s = make_hydran_hdn(e.name,pos,fac);    break;
        case Faction::Lyran:      s = make_lyran_ca(e.name,pos,fac);      break;
    }
    done:
    s.controller = e.controller;
    s.ship_class = e.ship_class;
    return s;
}

// â”€â”€ Seeking weapons â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€
struct SeekingWeapon {
    std::string  label;
    WeaponType   type;
    Hex          pos;
    int          target_ship;     // index into ships[]
    int          damage;          // base damage (plasma: attenuates; drones/fighters: fixed)
    int          speed;           // hexes per impulse
    Faction      owner_faction;
    int          owner_ship    = -1;  // index into ships[] for fighter recall
    int          hexes_travelled = 0; // FP1.5: plasma attenuation counter
    int          turns_remaining = -1; // drone endurance (-1 = unlimited)
    bool         spent = false;
};

static std::vector<SeekingWeapon> g_seekers; // global for the session

// E1.0: staged hit; all AI direct-fire is collected then applied simultaneously
struct PendingHit {
    int  ship_idx;
    int  damage;
    int  shield;
    bool hellbore = false;
    char log_msg[256] = {};
};

// At end of each game turn, surviving fighters return to their carrier bay (R9.F)
static void recall_fighters(std::vector<SeekingWeapon>& seekers, std::vector<Ship>& ships) {
    for (auto& sk : seekers) {
        if (sk.spent || sk.type != WeaponType::Fighter) continue;
        if (sk.owner_ship < 0 || sk.owner_ship >= (int)ships.size()) continue;
        Ship& carrier = ships[sk.owner_ship];
        if (carrier.destroyed) continue;
        // Return fighter to bay: re-enable the matching weapon
        for (auto& w : carrier.weapons) {
            if (w.type == WeaponType::Fighter && w.fired && !w.disabled) {
                w.fired   = false;
                w.armed   = false;
                w.charge  = 0;
                ++carrier.fighters_aboard;
                break;
            }
        }
        sk.spent = true; // remove from active seekers
    }
}

// Advance seekers one impulse: move toward target, detonate on contact
static void advance_seekers(std::vector<SeekingWeapon>& seekers,
                             std::vector<Ship>& ships,
                             char* msg_buf, int& ttl,
                             int& exp_ship, int& exp_frame,
                             std::deque<std::string>& combat_log) {
    for (auto& sk : seekers) {
        if (sk.spent) continue;
        if (sk.target_ship < 0 || sk.target_ship >= (int)ships.size()) { sk.spent = true; continue; }
        Ship& tgt = ships[sk.target_ship];
        if (tgt.destroyed) { sk.spent = true; continue; }

        // Move one hex toward target (simple greedy step)
        Hex prev_pos = sk.pos; // remember approach direction for shield selection
        int best_d = sk.pos.distance(tgt.position);
        Hex best_pos = sk.pos;
        for (int d = 0; d < 6; ++d) {
            Hex nb = hex_neighbour(sk.pos, d);
            int nd = nb.distance(tgt.position);
            if (nd < best_d) { best_d = nd; best_pos = nb; }
        }
        sk.pos = best_pos;

        ++sk.hexes_travelled;

        // Check contact
        if (sk.pos.q == tgt.position.q && sk.pos.r == tgt.position.r) {
            // D3.4: shield hit = approach direction relative to target facing
            int best_sh = 0, best_sd = 999;
            for (int d = 0; d < 6; ++d) {
                int dist = prev_pos.distance(hex_neighbour(tgt.position, d));
                if (dist < best_sd) { best_sd = dist; best_sh = d; }
            }
            int shield = (best_sh - tgt.facing + 6) % 6;
            // FP1.5: plasma damage attenuates with distance flown
            int hit_dmg = sk.damage;
            if (sk.type == WeaponType::PlasmaF || sk.type == WeaponType::PlasmaR)
                hit_dmg = std::max(0, sk.damage - 2 * sk.hexes_travelled);
            else if (sk.type == WeaponType::PlasmaG)
                hit_dmg = std::max(0, sk.damage - sk.hexes_travelled);
            tgt.apply_damage(hit_dmg, shield);
            sk.spent = true;
            std::snprintf(msg_buf, 256, "%s HITS %s: %d dmg (sh%d)",
                          sk.label.c_str(), tgt.name.c_str(), hit_dmg, shield + 1);
            ttl = 240;
            combat_log.push_back(msg_buf);
            if (combat_log.size() > 40) combat_log.pop_front();
            if (tgt.destroyed && !tgt.exploded) {
                exp_ship = sk.target_ship; exp_frame = 0;
            }
            if (!tgt.last_dac_msg.empty()) {
                std::snprintf(msg_buf, 256, "INTERNAL HIT on %s: %s",
                              tgt.name.c_str(), tgt.last_dac_msg.c_str());
                combat_log.push_back(msg_buf);
                if (combat_log.size() > 40) combat_log.pop_front();
            }
        }
    }
    // Erase spent seekers
    seekers.erase(std::remove_if(seekers.begin(), seekers.end(),
        [](const SeekingWeapon& s){ return s.spent; }), seekers.end());
}

// Launch a seeker from a ship firing a Drone, Plasma, or Fighter weapon
// Returns true if a seeker was spawned (caller should mark weapon fired)
static bool try_launch_seeker(Ship& atk, int weapon_idx,
                               const std::vector<Ship>& ships, int target_idx,
                               std::vector<SeekingWeapon>& seekers,
                               int owner_idx = -1) {
    if (weapon_idx < 0 || weapon_idx >= (int)atk.weapons.size()) return false;
    const Weapon& w = atk.weapons[weapon_idx];
    if (!w.can_fire()) return false;

    bool is_seeker = (w.type == WeaponType::Drone ||
                      w.type == WeaponType::PlasmaF ||
                      w.type == WeaponType::PlasmaR ||
                      w.type == WeaponType::PlasmaG ||
                      w.type == WeaponType::Fighter);
    if (!is_seeker) return false;
    if (w.type == WeaponType::Fighter && atk.fighters_aboard <= 0) return false;
    if (target_idx < 0 || target_idx >= (int)ships.size()) return false;
    if (ships[target_idx].destroyed) return false;

    // Damage stored at launch: drones/fighters fixed; plasma stores base (full value at r0)
    int dmg;
    if (w.type == WeaponType::Drone)          dmg = 6;   // Cadet rules Mk-I drone
    else if (w.type == WeaponType::Fighter)   dmg = 6;   // fighter attack
    else if (w.type == WeaponType::PlasmaF)   dmg = 20;  // FP1.5 full strength
    else if (w.type == WeaponType::PlasmaR)   dmg = 30;  // FP1.5 full strength
    else if (w.type == WeaponType::PlasmaG)   dmg = 10;  // FP1.5 full strength
    else                                       dmg = w.damage_at(1);

    int spd = (w.type == WeaponType::Drone)   ? 32  // standard drone: 32 hexes/turn
            : (w.type == WeaponType::Fighter) ? 12  // fighter movement rate
            : 2;  // plasma slower
    int endur = (w.type == WeaponType::Drone) ? 3 : -1; // drone: 3-turn endurance

    SeekingWeapon sk;
    sk.label         = atk.name + "/" + w.label;
    sk.type          = w.type;
    sk.pos           = atk.position;
    sk.target_ship   = target_idx;
    sk.damage        = dmg;
    sk.speed         = spd;
    sk.owner_faction   = atk.faction;
    sk.owner_ship      = owner_idx;
    sk.hexes_travelled = 0;
    sk.turns_remaining = endur;
    sk.spent           = false;
    seekers.push_back(sk);
    if (w.type == WeaponType::Fighter) --atk.fighters_aboard;
    // FD1.1: decrement drone rack ammo
    if (w.ammo > 0) --atk.weapons[weapon_idx].ammo;
    return true;
}

// â”€â”€ Combat helpers â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€
static int hit_facing(const Ship& attacker, const Ship& target) {
    int best = 0, best_dist = 999;
    for (int d = 0; d < 6; ++d) {
        int dist = attacker.position.distance(hex_neighbour(target.position, d));
        if (dist < best_dist) { best_dist = dist; best = d; }
    }
    return (best - target.facing + 6) % 6;
}

static std::string check_victory(const std::vector<Ship>& ships) {
    bool alive[9] = {};  // indexed by Faction enum (9 factions)
    for (auto& s : ships)
        if (!s.destroyed) alive[(int)s.faction] = true;

    int n = 0;
    for (bool a : alive) if (a) ++n;
    if (n > 1) return "";

    if (alive[(int)Faction::Federation]) return "FEDERATION";
    if (alive[(int)Faction::Klingon])    return "KLINGON EMPIRE";
    if (alive[(int)Faction::Romulan])    return "ROMULAN STAR EMPIRE";
    if (alive[(int)Faction::Gorn])       return "GORN HEGEMONY";
    if (alive[(int)Faction::Kzinti])     return "KZINTI HEGEMONY";
    if (alive[(int)Faction::Tholian])    return "THOLIAN ASSEMBLY";
    if (alive[(int)Faction::Orion])      return "ORION PIRATES";
    if (alive[(int)Faction::Hydran])     return "HYDRAN KINGDOMS";
    if (alive[(int)Faction::Lyran])      return "LYRAN STAR EMPIRE";
    return "NO SURVIVORS";
}

static void apply_explosion(Ship& src, std::vector<Ship>& ships,
                             char* msg_buf, int& ttl) {
    if (src.exploded) return;
    src.exploded = true;
    std::snprintf(msg_buf, 256, "*** %s EXPLODES! (power %d, r%d) ***",
                  src.name.c_str(), src.explosion_power(), src.explosion_radius());
    ttl = 300;
    for (auto& tgt : ships) {
        if (&tgt == &src || tgt.destroyed) continue;
        int dist = src.position.distance(tgt.position);
        int dmg  = src.explosion_damage_at(dist);
        if (dmg <= 0) continue;
        tgt.apply_damage(dmg, hit_facing(src, tgt));
    }
}

// â”€â”€ EAF commit helper (shared by human click and AI) â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€
static void commit_ship_eaf(Ship& s) {
    // H6.0/Battery: strip both pcap and battery_tap from remaining to get base surplus
    int pcap_used    = s.sys.pcap_charge;
    int base_surplus = s.power_remaining() - s.eaf.battery_tap - pcap_used;
    int net_battery  = std::max(0, base_surplus) - s.eaf.battery_tap;
    s.sys.battery_charge = std::clamp(s.sys.battery_charge + net_battery,
                                       0, s.sys.battery_cap);
    s.sys.pcap_charge = 0;  // H6.0: drain pcap; refilled by unfired phasers at begin_planning

    // B3.0: if over-budget, silently trim instant-weapon allocations from smallest up
    {
        int deficit = -s.power_remaining();
        for (auto& w : s.weapons) {
            if (deficit <= 0) break;
            if (!w.is_instant() || w.allocated <= 0) continue;
            int trim = std::min(deficit, w.allocated);
            w.allocated -= trim; deficit -= trim;
        }
    }

    // D6.123: sensor lock-on; auto-succeed if bridge OK; roll if damaged
    if (s.sys.bridge_ok) {
        s.sensor_lock = true;
    } else {
        int lock_roll = (std::rand() % 6) + 1;
        s.sensor_lock = (lock_roll >= 4);  // 50% chance with damaged bridge
    }

    // Apply cloak state for upcoming impulse phase
    s.sys.cloak_active = (s.eaf.cloak > 0);

    // Orion stealth bonus (G15.8): speed ≥ 24 grants +4 ECM automatically
    if (s.faction == Faction::Orion && s.eaf.speed >= 24)
        s.eaf.ecm += 4;

    // Orion engine doubling (G15.2): 1-in-3 chance of WarpEngine DAC hit
    if (s.eaf.engine_double) {
        int roll = std::rand() % 6;  // 0-5
        if (roll <= 1) {
            // Find first WarpEngine entry in DAC and apply it
            for (int i = s.dac_position; i < (int)s.dac.size(); ++i) {
                if (s.dac[i] == DACEntry::WarpEngine) {
                    s.apply_dac(i);
                    break;
                }
            }
        }
    }

    s.eaf_committed = true;
    for (auto& w : s.weapons) {
        if (!w.is_instant() && w.allocated > 0 && !w.armed) {
            if (++w.charge >= w.arming_turns) w.armed = true;
        } else if (!w.is_instant() && w.allocated == 0 && !w.armed) {
            w.charge = 0;
        }
    }
}

// â”€â”€ LLM bridge plan â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€

struct LlmPlan {
    std::string              posture;   // AGGRESSIVE | DEFENSIVE | EVASIVE | HOLD_RANGE
    int                      speed  = -1;
    int                      ecm    = -1;
    int                      repair = -1;
    std::vector<std::string> log;       // "Role: line" strings for display
    bool                     valid  = false;
};


static const char* faction_str(Faction f) {
    switch (f) {
    case Faction::Federation: return "FEDERATION";
    case Faction::Klingon:    return "KLINGON";
    case Faction::Romulan:    return "ROMULAN";
    case Faction::Gorn:       return "GORN";
    case Faction::Kzinti:     return "KZINTI";
    case Faction::Tholian:    return "THOLIAN";
    case Faction::Orion:      return "ORION";
    case Faction::Hydran:     return "HYDRAN";
    case Faction::Lyran:      return "LYRAN";
    default:                  return "UNKNOWN";
    }
}

// Escape a string so it is safe as a command-line argument on Windows
static std::string win_quote(const std::string& s) {
    return "\"" + s + "\"";
}

// SFB C3.31 Turn Mode Chart: cat 0=AA,1=A,2=B,3=C,4=D,5=E,6=F
static int turn_mode(int speed, int cat = 4) {
    if (speed <= 0) return 0;
    // rows: speed 1 | 2-5 | 6-10 | 11-15 | 16-20 | 21-25 | 26-30
    static const int tbl[7][7] = {
     // AA  A   B   C   D   E   F
        { 1, 1,  1,  1,  1,  1,  1 }, // speed 1
        { 2, 2,  2,  2,  2,  2,  2 }, // speed 2-5
        { 2, 2,  2,  3,  3,  4,  5 }, // speed 6-10
        { 3, 3,  4,  5,  6,  7,  8 }, // speed 11-15
        { 4, 5,  6,  7,  8,  9, 99 }, // speed 16-20 (F capped at 15)
        { 5, 6,  8, 10, 99, 99, 99 }, // speed 21-25 (D,E,F capped)
        { 6, 7, 99, 99, 99, 99, 99 }, // speed 26-30 (B+ capped)
    };
    int row = (speed == 1) ? 0
            : (speed <=  5) ? 1
            : (speed <= 10) ? 2
            : (speed <= 15) ? 3
            : (speed <= 20) ? 4
            : (speed <= 25) ? 5
            :                 6;
    int c = std::clamp(cat, 0, 6);
    return tbl[row][c];
}

static Hex forward_hex(const Ship& s) { return hex_neighbour(s.position, s.facing); }

// Windows file dialogs for scenario save/load
static std::string dialog_save_path(HWND parent) {
    char buf[MAX_PATH] = "scenario.json";
    OPENFILENAMEA ofn = {};
    ofn.lStructSize   = sizeof(ofn);
    ofn.hwndOwner     = parent;
    ofn.lpstrFilter   = "JSON Scenario\0*.json\0All Files\0*.*\0";
    ofn.lpstrFile     = buf;
    ofn.nMaxFile      = MAX_PATH;
    ofn.lpstrDefExt   = "json";
    ofn.lpstrTitle    = "Save Scenario";
    ofn.Flags         = OFN_OVERWRITEPROMPT | OFN_NOCHANGEDIR;
    return GetSaveFileNameA(&ofn) ? std::string(buf) : std::string{};
}

static std::string dialog_load_path(HWND parent) {
    char buf[MAX_PATH] = {};
    OPENFILENAMEA ofn = {};
    ofn.lStructSize   = sizeof(ofn);
    ofn.hwndOwner     = parent;
    ofn.lpstrFilter   = "JSON Scenario\0*.json\0All Files\0*.*\0";
    ofn.lpstrFile     = buf;
    ofn.nMaxFile      = MAX_PATH;
    ofn.lpstrTitle    = "Load Scenario";
    ofn.Flags         = OFN_FILEMUSTEXIST | OFN_NOCHANGEDIR;
    return GetOpenFileNameA(&ofn) ? std::string(buf) : std::string{};
}

// Persisted crew names: ship-name -> {helm, weapons, engineering, science}
static std::map<std::string, std::array<std::string,4>> g_crew_roster;

// Fetch (or generate) crew names for a ship via sfb_names.py.
// Stores result in g_crew_roster[ship_name]. No-op if already loaded.
static void ensure_crew_names(const Ship& s) {
    if (g_crew_roster.count(s.name)) return;
    #pragma warning(suppress: 4996)
    const char* tmp = std::getenv("TEMP");
    if (!tmp) tmp = "C:\\Temp";
    std::string out_path = std::string(tmp) + "\\sfb_names_out.txt";
    std::string script = "C:\\Users\\jonat\\Projects\\Star Fleet Battles\\sfb_names.py";
    std::string faction = faction_str(s.faction);
    for (auto& c : faction) c = static_cast<char>(std::toupper(static_cast<unsigned char>(c)));
    std::string cmd = "python " + win_quote(script) + " get "
                    + win_quote(s.name) + " " + win_quote(faction)
                    + " " + win_quote(out_path) + " 2>nul";
    if (std::system(cmd.c_str()) != 0) return;
    std::ifstream f(out_path);
    if (!f) return;
    std::array<std::string,4> crew;
    const char* keys[4] = {"HELM", "WEAPONS", "ENGINEERING", "SCIENCE"};
    std::string line;
    while (std::getline(f, line)) {
        auto eq = line.find('=');
        if (eq == std::string::npos) continue;
        std::string key = line.substr(0, eq);
        std::string val = line.substr(eq + 1);
        for (int i = 0; i < 4; ++i)
            if (key == keys[i]) crew[i] = val;
    }
    g_crew_roster[s.name] = crew;
}

// Update a single crew name in the roster (saves to JSON via sfb_names.py update)
static void update_crew_name(const std::string& ship_name, int role_idx, const std::string& new_name) {
    const char* role_keys[4] = {"helm","weapons","engineering","science"};
    #pragma warning(suppress: 4996)
    const char* tmp = std::getenv("TEMP");
    if (!tmp) tmp = "C:\\Temp";
    std::string out_path = std::string(tmp) + "\\sfb_names_upd.txt";
    std::string script = "C:\\Users\\jonat\\Projects\\Star Fleet Battles\\sfb_names.py";
    std::string cmd = "python " + win_quote(script) + " update "
                    + win_quote(ship_name) + " " + win_quote(role_keys[role_idx])
                    + " " + win_quote(new_name) + " " + win_quote(out_path) + " 2>nul";
    std::system(cmd.c_str());
    if (g_crew_roster.count(ship_name))
        g_crew_roster[ship_name][role_idx] = new_name;
}

// Build JSON battle state for a single ship
static std::string build_battle_json(const Ship& s, const std::vector<Ship>& ships,
                                     int turn, int max_turns) {
    std::ostringstream j;
    j << "{\"ship\":{\"name\":\"" << s.name << "\",\"faction\":\"" << faction_str(s.faction) << "\"";
    j << ",\"hull\":" << s.sys.hull << ",\"hull_max\":" << s.sys.hull_max;
    j << ",\"power\":" << s.total_power();
    j << ",\"shields\":[";
    for (int i = 0; i < 6; ++i) j << s.sys.shields[i] << (i<5?",":"");
    j << "],\"shields_max\":[";
    for (int i = 0; i < 6; ++i) j << s.sys.shields_max[i] << (i<5?",":"");
    j << "],\"weapons\":[";
    bool first = true;
    for (auto& w : s.weapons) {
        if (!first) j << ",";
        first = false;
        j << "{\"label\":\"" << w.label << "\",\"type\":\"" << weapon_name(w.type) << "\""
          << ",\"armed\":" << (w.armed ? "true" : "false")
          << ",\"charge\":" << w.charge
          << ",\"arming_turns\":" << w.arming_turns
          << ",\"disabled\":" << (w.disabled ? "true" : "false") << "}";
    }
    j << "]";
    // Inject crew names if available
    if (g_crew_roster.count(s.name)) {
        const auto& crew = g_crew_roster.at(s.name);
        const char* roles[4] = {"helm","weapons","engineering","science"};
        j << ",\"crew\":{";
        for (int i = 0; i < 4; ++i) {
            if (i) j << ",";
            j << "\"" << roles[i] << "\":\"" << crew[i] << "\"";
        }
        j << "}";
    }
    j << "},\"enemies\":[";
    first = true;
    for (auto& e : ships) {
        if (e.faction == s.faction || e.destroyed) continue;
        if (!first) j << ",";
        first = false;
        int range    = s.position.distance(e.position);
        int hull_pct = e.sys.hull * 100 / std::max(1, e.sys.hull_max);
        j << "{\"name\":\"" << e.name << "\",\"faction\":\"" << faction_str(e.faction)
          << "\",\"range\":" << range << ",\"hull_pct\":" << hull_pct << "}";
    }
    j << "],\"turn\":" << turn << ",\"max_turns\":" << max_turns << "}";
    return j.str();
}

static LlmPlan call_llm(const Ship& s, const std::vector<Ship>& ships,
                         int turn, int max_turns) {
    LlmPlan plan;

    #pragma warning(suppress: 4996)
    const char* tmp = std::getenv("TEMP");
    if (!tmp) tmp = "C:\\Temp";
    std::string in_path  = std::string(tmp) + "\\sfb_plan_in.json";
    std::string out_path = std::string(tmp) + "\\sfb_plan_out.txt";

    // Write state JSON
    {
        std::ofstream f(in_path);
        if (!f) return plan;
        f << build_battle_json(s, ships, turn, max_turns);
    }

    // Locate sfb_ai.py relative to the executable (build dir)
    // Executable is in build/, script is one level up in project root
    std::string script = "C:\\Users\\jonat\\Projects\\Star Fleet Battles\\sfb_ai.py";
    std::string cmd = "python " + win_quote(script) + " "
                    + win_quote(in_path) + " " + win_quote(out_path) + " 2>nul";
    if (std::system(cmd.c_str()) != 0) return plan;

    // Parse response
    std::ifstream f(out_path);
    if (!f) return plan;
    std::string line;
    while (std::getline(f, line)) {
        auto eq = line.find('=');
        if (eq == std::string::npos) continue;
        std::string key = line.substr(0, eq);
        std::string val = line.substr(eq + 1);
        if (key == "POSTURE") plan.posture = val;
        else if (key == "SPEED")  try { plan.speed  = std::stoi(val); } catch (...) {}
        else if (key == "ECM")    try { plan.ecm    = std::stoi(val); } catch (...) {}
        else if (key == "REPAIR") try { plan.repair = std::stoi(val); } catch (...) {}
        else if (key == "LOG")    plan.log.push_back(val);
    }
    plan.valid = !plan.posture.empty();
    return plan;
}


static CrewAdvice call_crew_llm(const Ship& s, const std::vector<Ship>& ships,
                                 int turn, int max_turns) {
    CrewAdvice advice;

    #pragma warning(suppress: 4996)
    const char* tmp = std::getenv("TEMP");
    if (!tmp) tmp = "C:\\Temp";
    std::string in_path  = std::string(tmp) + "\\sfb_crew_in.json";
    std::string out_path = std::string(tmp) + "\\sfb_crew_out.txt";

    { std::ofstream f(in_path); if (!f) return advice; f << build_battle_json(s, ships, turn, max_turns); }

    std::string script = "C:\\Users\\jonat\\Projects\\Star Fleet Battles\\sfb_crew.py";
    std::string cmd = "python " + win_quote(script) + " "
                    + win_quote(in_path) + " " + win_quote(out_path) + " 2>nul";
    if (std::system(cmd.c_str()) != 0) return advice;

    std::ifstream f(out_path);
    if (!f) return advice;
    std::string line;
    while (std::getline(f, line)) {
        auto eq = line.find('=');
        if (eq == std::string::npos) continue;
        std::string key = line.substr(0, eq);
        std::string val = line.substr(eq + 1);
        if      (key == "HELM")        advice.helm        = val;
        else if (key == "WEAPONS")     advice.weapons     = val;
        else if (key == "ENGINEERING") advice.engineering = val;
        else if (key == "SCIENCE")     advice.science     = val;
        else if (key == "CHARGE")      advice.charge_weapon = val;
        else if (key == "SPEED")       try { advice.speed = std::stoi(val); } catch (...) {}
        else if (key == "CLOAK")       advice.recommend_cloak = (val == "1");
    }
    advice.valid = !advice.helm.empty();
    return advice;
}
// â”€â”€ Racial AI profiles â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€
struct AiProfile {
    int  preferred_range;   // ideal combat distance in hexes
    int  retreat_hull_pct;  // bail-out threshold (% of hull_max)
    bool use_ecm;
    int  speed_close;       // power fraction (/10) for speed when closing
    int  speed_hold;        // power fraction (/10) for speed when holding range
};

static AiProfile base_faction_profile(Faction f) {
    switch (f) {
    case Faction::Federation: return {5, 25, true,  5, 3};
    case Faction::Klingon:    return {2, 15, false, 7, 5};
    case Faction::Romulan:    return {7, 30, true,  4, 2};
    case Faction::Gorn:       return {3, 20, false, 4, 3};
    case Faction::Kzinti:     return {3, 20, false, 6, 4};
    case Faction::Tholian:    return {5, 35, false, 3, 2}; // slow, defensive web-spinners
    case Faction::Orion:      return {4, 20, true,  6, 4}; // opportunistic pirates
    case Faction::Hydran:     return {5, 25, false, 5, 3}; // balanced hellbore/gatling mix
    case Faction::Lyran:      return {2, 15, false, 7, 5}; // aggressive, like Klingons
    default:                  return {4, 25, false, 5, 3};
    }
}

// Adjusts base faction profile for actual ship characteristics
static AiProfile compute_ai_profile(const Ship& s) {
    AiProfile p = base_faction_profile(s.faction);

    // Hull class: heavier ships are more aggressive, lighter ones bail sooner
    int hull_max = s.sys.hull_max;
    if (hull_max >= 150)      { p.retreat_hull_pct = std::max(0, p.retreat_hull_pct - 10); }
    else if (hull_max >= 100) { /* base */ }
    else if (hull_max >= 60)  { p.retreat_hull_pct += 5; }
    else                      { p.retreat_hull_pct += 10; p.preferred_range += 1; }

    // Weapon mix: check what this ship actually carries
    bool has_plasma = false, has_instant = false, has_long_range = false;
    int  max_reach = 0;
    for (auto& w : s.weapons) {
        if (w.disabled) continue;
        if (w.type == WeaponType::PlasmaF || w.type == WeaponType::PlasmaR ||
            w.type == WeaponType::PlasmaG)  has_plasma = true;
        if (w.is_instant())                 has_instant = true;
        // Find effective range cutoff
        for (int r = 30; r >= 0; --r)
            if (w.damage_at(r) > 0) { max_reach = std::max(max_reach, r); break; }
        if (max_reach >= 10) has_long_range = true;
    }

    // Plasma-primary ships want to open to plasma effective range
    if (has_plasma && !has_instant) p.preferred_range = std::max(p.preferred_range, 5);
    // Ships with short-range-only weapons (fusion, close disruptors) prefer to close
    if (!has_long_range && max_reach <= 6) p.preferred_range = std::min(p.preferred_range, 3);
    // Fast ships (high max speed) can afford to be more aggressive
    if (s.sys.max_warp_power >= 30) p.speed_close = std::min(8, p.speed_close + 1);

    return p;
}

// â”€â”€ AI energy allocation â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€
static void ai_eaf(Ship& s, const std::vector<Ship>& ships, const LlmPlan* plan = nullptr) {
    for (int i = 0; i < 6; ++i) s.eaf.reinforce[i] = 0;
    s.eaf.ecm = s.eaf.eccm = s.eaf.repair = s.eaf.lab =
    s.eaf.tractors = s.eaf.transporters = 0;
    s.eaf.warp_power    = s.sys.max_warp_power;
    s.eaf.impulse_power = s.sys.max_impulse_power;
    for (auto& w : s.weapons) if (w.is_instant()) w.allocated = 0;

    AiProfile p = compute_ai_profile(s);
    int hull_pct  = s.sys.hull * 100 / std::max(1, s.sys.hull_max);
    bool retreating = hull_pct < p.retreat_hull_pct;

    // LLM posture overrides
    if (plan && plan->valid) {
        if (plan->posture == "AGGRESSIVE") { p.speed_close += 2; p.preferred_range -= 1; }
        else if (plan->posture == "DEFENSIVE") { p.speed_hold -= 1; p.retreat_hull_pct += 10; }
        else if (plan->posture == "EVASIVE") { retreating = true; }
        // HOLD_RANGE: no change to profile
    }

    // Nearest enemy (for range awareness and shield facing)
    const Ship* threat = nullptr;
    int threat_dist = 999;
    for (auto& o : ships) {
        if (&o == &s || o.destroyed || o.faction == s.faction) continue;
        int d = s.position.distance(o.position);
        if (d < threat_dist) { threat_dist = d; threat = &o; }
    }

    // Mandatory costs (A2.0 + A5.0) are pre-deducted from the AI budget
    int power = s.total_power() - s.mandatory_power();

    // D9.0 repair: check shield damage; allocate in multiples of 2
    int shield_damage = 0;
    for (int i = 0; i < 6; ++i)
        shield_damage += std::max(0, s.sys.shields_max[i] - s.sys.shields[i]);
    if (plan && plan->valid && plan->repair >= 0) {
        int rp = std::min(plan->repair, (power / 2) * 2);
        s.eaf.repair = rp; power -= rp;
    } else if (shield_damage > 0 && power >= 2) {
        int want = std::min(retreating ? 8 : 6, shield_damage);
        int rp   = std::min(want, (power / 2) * 2);
        s.eaf.repair = rp; power -= rp;
    }

    // Arm non-instant weapons (skip if retreating and power-starved)
    if (!retreating || power > 12) {
        for (auto& w : s.weapons) {
            if (!w.is_instant() && !w.armed && !w.disabled && power >= w.arming_cost) {
                w.allocated = w.arming_cost;
                power -= w.arming_cost;
            }
        }
    }

    // Speed: fraction depends on range posture
    int sfrac = retreating ? 8
              : (threat_dist > p.preferred_range + 1) ? p.speed_close
              : p.speed_hold;
    // C2.2: accel cap; C2.12: APR excluded from movement budget
    int accel_max  = std::min(31, s.last_speed + std::max(s.last_speed, 10));
    int move_avail = std::min(accel_max, s.movement_power());
    int base_spd   = std::clamp(power * sfrac / 10, 0, move_avail);
    if (plan && plan->valid && plan->speed >= 0)
        s.eaf.speed = std::clamp(plan->speed, 0, std::min(std::min(31, s.last_speed + std::max(s.last_speed, 10)), s.movement_power()));
    else
        s.eaf.speed = base_spd;
    power -= s.eaf.speed;

    // ECM: factions that value it, or LLM override
    int ecm_want = (plan && plan->valid && plan->ecm >= 0) ? plan->ecm
                 : (p.use_ecm && !retreating) ? 2 : 0;
    if (ecm_want > 0 && power >= ecm_want) {
        s.eaf.ecm = std::min(ecm_want, power);
        power -= s.eaf.ecm;
    }
    // D3.341: AI allocates general reinforce when damaged and has spare power
    if (shield_damage > 0 && power >= 2) {
        int gen_want = std::min(4, (power / 2) * 2);  // up to 4 pts = 2 absorb
        s.eaf.general_reinforce = gen_want;
        power -= gen_want;
    }

    // Phasers / instant weapons: fill to max
    for (auto& w : s.weapons) {
        if (w.is_instant() && !w.disabled && power > 0) {
            int alloc = std::min(power, w.max_power);
            w.allocated = alloc;
            power -= alloc;
        }
    }

    // Cloak: Romulans when badly hurt
    if (s.sys.cloak_installed && hull_pct < 30) {
        s.eaf.cloak = s.sys.cloak_cost;
        power = std::max(0, power - s.sys.cloak_cost);
    }

    // Reinforce the shield that actually faces the threat
    if (power > 0) {
        int idx = threat ? hit_facing(*threat, s) : 0;
        s.eaf.reinforce[idx] = std::min(power, s.sys.shields[idx]);
    }
}

static void advance_eaf_player(int& eaf_player, std::vector<Ship>& ships,
                                GamePhase& phase, int& impulse,
                                std::vector<LlmPlan>& llm_plans, int turn, int max_turns,
                                bool llm_on, std::deque<std::string>& /*combat_log*/) {
    while (eaf_player < (int)ships.size() &&
           (ships[eaf_player].destroyed ||
            ships[eaf_player].controller == ShipController::AI))
    {
        if (!ships[eaf_player].destroyed) {
            LlmPlan* plan = nullptr;
            if (llm_on && eaf_player < (int)llm_plans.size()) {
                llm_plans[eaf_player] = call_llm(ships[eaf_player], ships, turn, max_turns);
                plan = &llm_plans[eaf_player];
                // AI internal plans are not shown in the combat log (only player crew advice is)
            }
            ai_eaf(ships[eaf_player], ships, plan);
            commit_ship_eaf(ships[eaf_player]);
        }
        ++eaf_player;
    }

    if (eaf_player >= (int)ships.size()) {
        phase      = GamePhase::Impulse;
        eaf_player = 0;
        impulse    = 1;
        for (auto& ship : ships)
            if (!ship.destroyed)
                ship.move_available = moves_this_impulse(ship.eaf.speed, 1);
    }
}

// â”€â”€ AI impulse actions â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€
static void ai_move(Ship& ship, const std::vector<Ship>& ships, const Board& board,
                    const LlmPlan* plan = nullptr) {
    if (!ship.move_available) return;

    AiProfile p = compute_ai_profile(ship);
    int hull_pct  = ship.sys.hull * 100 / std::max(1, ship.sys.hull_max);
    bool retreating = hull_pct < p.retreat_hull_pct;

    if (plan && plan->valid) {
        if (plan->posture == "AGGRESSIVE") { p.preferred_range -= 1; }
        else if (plan->posture == "EVASIVE") { retreating = true; }
        else if (plan->posture == "HOLD_RANGE") { /* unchanged */ }
    }

    // Collect enemies â€” sort by kill-score (most wounded first as primary threat)
    struct EnemyInfo { const Ship* s; int idx; float kill_score; int dist; };
    std::vector<EnemyInfo> enemies;
    for (int i = 0; i < (int)ships.size(); ++i) {
        auto& s = ships[i];
        if (&s == &ship || s.destroyed || s.faction == ship.faction) continue;
        float dmg_pct = 1.f - (float)s.sys.hull / std::max(1, s.sys.hull_max);
        int d = ship.position.distance(s.position);
        enemies.push_back({&s, i, dmg_pct * 10.f - d * 0.1f, d});
    }
    if (enemies.empty()) { ship.move_available = false; return; }
    std::sort(enemies.begin(), enemies.end(), [](auto& a, auto& b){
        return a.kill_score > b.kill_score;
    });

    // Primary target for facing
    // Choose best facing by scoring what each direction would position us next impulse
    int best_face = ship.facing;
    float best_face_val = -1e9f;
    for (int dir = 0; dir < 6; ++dir) {
        Hex probe = hex_neighbour(ship.position, dir);
        float val = 0.f;
        for (int ei = 0; ei < (int)enemies.size(); ++ei) {
            float w = (ei == 0) ? 3.f : (ei == 1) ? 0.5f : 0.25f;
            int nd = probe.distance(enemies[ei].s->position);
            val += retreating ? w*(float)nd
                              : w*(10.f - (float)std::abs(nd - p.preferred_range));
        }
        if (val > best_face_val) { best_face_val = val; best_face = dir; }
    }

    // Turn at most once per impulse if turn mode allows
    int ai_tm = turn_mode(ship.eaf.speed, ship.turn_mode_cat);
    if (ship.facing != best_face && ship.hexes_since_turn >= ai_tm) {
        int cw  = (best_face - ship.facing + 6) % 6;
        int ccw = (ship.facing - best_face + 6) % 6;
        ship.facing = (cw <= ccw) ? (ship.facing + 1) % 6 : (ship.facing + 5) % 6;
        ship.hexes_since_turn = 0;
    }

    // Move into forward hex if clear
    Hex ai_fwd = forward_hex(ship);
    bool ai_can = board.contains(ai_fwd);
    if (ai_can) {
        Terrain ai_t = board.at(ai_fwd).terrain;
        if (ai_t == Terrain::Asteroid || ai_t == Terrain::Planet ||
            ai_t == Terrain::TholianWeb || ai_t == Terrain::BlackHole) ai_can = false;
    }
    if (ai_can) {
        for (auto& s : ships)
            if (!s.destroyed && &s != &ship &&
                s.position.q == ai_fwd.q && s.position.r == ai_fwd.r) { ai_can = false; break; }
    }
    if (ai_can) {
        ship.position = ai_fwd;
        ++ship.moves_used;
        ++ship.hexes_since_turn;
    }
    ship.move_available = false;
}

static void ai_fire(Ship& atk, std::vector<Ship>& ships, const Renderer& rend,
                    char* msg_buf, int& ttl, int& exp_ship, int& exp_frame,
                    int atk_idx, std::vector<SeekingWeapon>& seekers,
                    std::deque<std::string>& combat_log,
                    std::vector<PendingHit>& pending,   // E1.0: staged fire
                    const Board& board)                 // P9: dust cloud checks
{
    if (atk.sys.cloak_active) return;

    // Fire order: highest-damage weapon first
    std::vector<int> order;
    for (int i = 0; i < (int)atk.weapons.size(); ++i)
        if (atk.weapons[i].can_fire()) order.push_back(i);
    std::sort(order.begin(), order.end(), [&](int a, int b) {
        auto& wa = atk.weapons[a]; auto& wb = atk.weapons[b];
        int da = wa.is_instant() ? wa.allocated : wa.max_power;
        int db = wb.is_instant() ? wb.allocated : wb.max_power;
        return da > db;
    });

    for (int wi : order) {
        Weapon& w = atk.weapons[wi];
        if (!w.can_fire()) continue;
        int ai_die = (rand() % 6) + 1;  // rolled once per weapon fire

        bool is_seeking = (w.type == WeaponType::Drone ||
                           w.type == WeaponType::PlasmaF ||
                           w.type == WeaponType::PlasmaR ||
                           w.type == WeaponType::PlasmaG ||
                           w.type == WeaponType::Fighter);

        // Best target: maximise damage / remaining hull (kill progress)
        int best_tgt = -1;
        float best_score = -1.f;
        for (int i = 0; i < (int)ships.size(); ++i) {
            auto& s = ships[i];
            if (s.destroyed || s.faction == atk.faction) continue;
            if (!is_seeking && !rend.hex_in_arc(atk.position, atk.facing, w.arc, s.position)) continue;
            int range = atk.position.distance(s.position);
            int dmg   = is_seeking ? w.damage_at(range) : w.roll_damage(range, ai_die);
            if (dmg <= 0) dmg = is_seeking ? w.damage_at(1) : 0;
            if (dmg <= 0) continue;
            if (w.is_instant()) {
                int net = std::max(0, s.eaf.ecm - atk.eaf.eccm);
                dmg = std::max(0, dmg - net);
            }
            if (dmg <= 0) continue;
            float score = (float)dmg / std::max(1, s.sys.hull);
            if (score > best_score) { best_score = score; best_tgt = i; }
        }
        if (best_tgt < 0) continue;

        Ship& tgt = ships[best_tgt];
        int range = atk.position.distance(tgt.position);
        if (!atk.sensor_lock) range *= 2;  // D6.123

        // Seeking weapons: launch and track
        if (is_seeking) {
            try_launch_seeker(atk, wi, ships, best_tgt, seekers, atk_idx);
            w.fired = true;
            if (!w.is_instant()) { w.armed = false; w.charge = 0; }
            std::snprintf(msg_buf, 256, "AI %s launches %s -> %s",
                          atk.name.c_str(), w.label.c_str(), tgt.name.c_str());
            ttl = 180;
            combat_log.push_back(msg_buf);
            if (combat_log.size() > 40) combat_log.pop_front();
            continue;
        }

        // ESG: area weapon — fire against ALL enemies in radius simultaneously
        if (w.type == WeaponType::ESG) {
            int hits = 0;
            w.fired = true;
            w.stored = 0;  // G23.0: discharge accumulated energy
            for (auto& enemy : ships) {
                if (enemy.destroyed || enemy.faction == atk.faction) continue;
                int er = atk.position.distance(enemy.position);
                int ed = w.damage_at(er);
                if (ed <= 0) continue;
                int sh = hit_facing(atk, enemy);
                enemy.apply_damage(ed, sh);
                ++hits;
            }
            std::snprintf(msg_buf, 256, "AI %s fires ESG (str %d) — %d ship(s) in radius",
                          atk.name.c_str(), w.allocated, hits);
            ttl = 240;
            combat_log.push_back(msg_buf);
            if (combat_log.size() > 40) combat_log.pop_front();
            continue;
        }

        int dmg   = w.roll_damage(range, ai_die);  // same die used for target selection
        // P9 Dust Cloud: phaser effective range +5
        if (w.is_instant()) {
            bool in_dust = (board.at(atk.position).terrain == Terrain::DustCloud ||
                            board.at(tgt.position).terrain == Terrain::DustCloud);
            if (in_dust) dmg = w.roll_damage(range + 5, ai_die);
        }
        int ecm_r = 0;
        if (w.is_instant()) {
            int net = std::max(0, tgt.eaf.ecm - atk.eaf.eccm);
            // D13.0 Aegis fire control: ECM halved at close range (<=3), 1.5x at extreme range (>=9)
            if (range <= 3) net = net / 2;
            else if (range >= 9) net = net * 3 / 2;
            ecm_r = std::min(dmg, net);
            dmg   = std::max(0, dmg - ecm_r);
        }
        int shield = hit_facing(atk, tgt);
        w.fired = true;
        if (!w.is_instant()) { w.armed = false; w.charge = 0; }
        // E1.0: stage hit instead of applying immediately
        PendingHit ph;
        ph.ship_idx = best_tgt;
        ph.damage   = dmg;
        ph.shield   = shield;
        ph.hellbore = (w.type == WeaponType::Hellbore);
        std::snprintf(ph.log_msg, sizeof(ph.log_msg),
            "AI %s fires %s -> %s: %d dmg #%d (r%d d%d)%s%s",
            atk.name.c_str(), w.label.c_str(), tgt.name.c_str(),
            dmg, shield+1, range, ai_die, ecm_r ? " [ECM]" : "",
            ph.hellbore ? " [bypass]" : "");
        pending.push_back(ph);
    }
}

// â”€â”€ Process all AI ships for this impulse â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€
static void run_ai_impulse(std::vector<Ship>& ships, const Board& board,
                            const Renderer& rend, int impulse,
                            char* msg_buf, int& ttl,
                            int& exp_ship, int& exp_frame,
                            const std::vector<LlmPlan>& llm_plans,
                            std::vector<SeekingWeapon>& seekers,
                            std::deque<std::string>& combat_log) {
    std::vector<PendingHit> pending;  // E1.0: collect all AI fire before applying
    for (int i = 0; i < (int)ships.size(); ++i) {
        Ship& s = ships[i];
        if (s.destroyed || s.controller != ShipController::AI) continue;
        s.move_available = moves_this_impulse(s.eaf.speed, impulse);
        const LlmPlan* plan = (i < (int)llm_plans.size()) ? &llm_plans[i] : nullptr;
        if (s.move_available) ai_move(s, ships, board, plan);
        ai_fire(s, ships, rend, msg_buf, ttl, exp_ship, exp_frame, i, seekers, combat_log, pending, board);
    }
    // E1.0: apply all staged hits simultaneously
    for (auto& ph : pending) {
        if (ph.ship_idx < 0 || ph.ship_idx >= (int)ships.size()) continue;
        Ship& tgt = ships[ph.ship_idx];
        if (ph.hellbore)
            tgt.apply_damage_hellbore(ph.damage, ph.shield);
        else
            tgt.apply_damage(ph.damage, ph.shield);
        std::strncpy(msg_buf, ph.log_msg, 255);
        ttl = 240;
        combat_log.push_back(msg_buf);
        if (combat_log.size() > 40) combat_log.pop_front();
        if (!tgt.last_dac_msg.empty()) {
            std::snprintf(msg_buf, 256, "INTERNAL HIT on %s: %s",
                          tgt.name.c_str(), tgt.last_dac_msg.c_str());
            ttl = 300;
            combat_log.push_back(msg_buf);
            if (combat_log.size() > 40) combat_log.pop_front();
        }
        if (tgt.destroyed && !tgt.exploded) {
            exp_ship = ph.ship_idx; exp_frame = 0;
            apply_explosion(tgt, ships, msg_buf, ttl);
        }
    }
}

// â”€â”€ main â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€
int main(int, char**) {
    if (SDL_Init(SDL_INIT_VIDEO) != 0) return 1;
    if (TTF_Init() != 0) { SDL_Quit(); return 1; }

    int W = 1280, H = 900;
    SDL_Window* win = SDL_CreateWindow(
        "Star Fleet Battles",
        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
        W, H, SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE);
    if (!win) { TTF_Quit(); SDL_Quit(); return 1; }

    // Get native HWND for file dialogs
    HWND hwnd = nullptr;
    {
        SDL_SysWMinfo wmi;
        SDL_VERSION(&wmi.version);
        if (SDL_GetWindowWMInfo(win, &wmi))
            hwnd = wmi.info.win.window;
    }

    SDL_Renderer* ren = SDL_CreateRenderer(win, -1, SDL_RENDERER_SOFTWARE);
    if (!ren) { SDL_DestroyWindow(win); TTF_Quit(); SDL_Quit(); return 1; }

    TTF_Font* font = TTF_OpenFont("C:/Windows/Fonts/arial.ttf", 14);

    Board board(8);
    std::string assets_dir = "assets/ssd";
    if (char* base = SDL_GetBasePath()) {
        assets_dir = std::string(base) + "assets/ssd";
        SDL_free(base);
    }
    Renderer rend(ren, font, W - SIDEBAR_W, H, 40.0f, assets_dir);

    // â”€â”€ State â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€
    GamePhase   phase      = GamePhase::Setup;
    std::vector<SetupEntry> setup = default_setup();
    std::vector<Renderer::SetupButton> setup_buttons;
    int setup_dropdown_row    = -1; // which row has class dropdown open (-1=none)
    int setup_dropdown_scroll =  0; // scroll offset within that dropdown
    bool pause_menu_open = false;   // Esc during battle opens pause overlay
    std::vector<Renderer::PauseButton> pause_buttons;

    std::vector<Ship> ships;
    int eaf_player = 0, eaf_scroll = 0, turn = 1, impulse = 1;
    bool eaf_hidden = false;
    int  terrain_brush = 0;
    int  rift_dir_brush = 0;  // GravityRift direction (0-5), cycled with R key
    int  placement_idx = 0;
    float hex_size = 40.0f;
    int max_turns  = 8;
    bool llm_on    = true;   // toggle with [L] key
    std::vector<LlmPlan>   llm_plans;
    std::vector<CrewAdvice> crew_advices;  // per-ship crew advisory for player ships
    int selected = -1, selected_weapon = -1;
    bool fire_mode = false;
    std::string winner;

    // Crew name editing state
    int  crew_edit_role = -1;   // which role row is being edited (-1 = none)
    std::string crew_edit_buf;  // accumulated input text
    std::vector<SDL_Rect> crew_row_rects; // hit-test rects from last draw_crew_advice

    int  exp_ship = -1, exp_frame = 0;
    char msg_buf[256] = "";
    int  msg_ttl = 0;
    std::deque<std::string> combat_log;
    auto push_log = [&]() {
        if (msg_buf[0]) combat_log.push_back(msg_buf);
        if (combat_log.size() > 40) combat_log.pop_front();
    };

    std::vector<EafButton> eaf_buttons, weapon_buttons;

    bool show_ssd = false;
    // Map panning (right-mouse drag)
    bool panning = false;
    int pan_last_x = 0, pan_last_y = 0;
    bool running = true;
    int board_area_w = W - SIDEBAR_W;
    while (running) {
        SDL_GetWindowSize(win, &W, &H);
        board_area_w = W - SIDEBAR_W;
        rend.set_screen_size(board_area_w, H);
        SDL_Event ev;
        while (SDL_PollEvent(&ev)) {
            // Map pan: right-mouse drag
            if (ev.type == SDL_MOUSEBUTTONDOWN && ev.button.button == SDL_BUTTON_RIGHT) {
                panning = true; pan_last_x = ev.button.x; pan_last_y = ev.button.y;
            }
            if (ev.type == SDL_MOUSEBUTTONUP && ev.button.button == SDL_BUTTON_RIGHT) {
                panning = false;
            }
            if (ev.type == SDL_MOUSEMOTION && panning && ev.motion.x < board_area_w) {
                rend.pan((float)(ev.motion.x - pan_last_x),
                         (float)(ev.motion.y - pan_last_y));
                pan_last_x = ev.motion.x;
                pan_last_y = ev.motion.y;
            }
            if (ev.type == SDL_QUIT) { running = false; break; }
            // Ctrl+Q: keyboard quit shortcut (Alt+F4 also works)
            if (ev.type == SDL_KEYDOWN &&
                ev.key.keysym.sym == SDLK_q &&
                (ev.key.keysym.mod & KMOD_CTRL)) { running = false; break; }

            // Global S/L: save/load scenario from any phase
            if (ev.type == SDL_KEYDOWN && ev.key.keysym.sym == SDLK_s &&
                !(ev.key.keysym.mod & KMOD_CTRL)) {
                std::string spath = dialog_save_path(hwnd);
                if (!spath.empty()) {
                    std::ofstream sf(spath);
                    if (sf) {
                        sf << "{" << '"' << "max_turns" << '"' << ":" << max_turns
                           << "," << '"' << "ships" << '"' << ":[";
                        for (int i = 0; i < (int)setup.size(); ++i) {
                            const auto& e = setup[i];
                            if (i) sf << ",";
                            char q = '"';  // quote char helper
                            sf << "{" << q << "name"       << q << ":" << q << e.name       << q
                               << "," << q << "ship_class" << q << ":" << q << e.ship_class << q
                               << "," << q << "faction"    << q << ":" << (int)e.faction
                               << "," << q << "included"   << q << ":" << (e.included ? "true" : "false")
                               << "," << q << "controller" << q << ":" << (int)e.controller
                               << "," << q << "class_idx"  << q << ":" << e.class_idx << "}";
                        }
                        sf << "]}";
                    }
                }
                continue;
            }
            if (ev.type == SDL_KEYDOWN && ev.key.keysym.sym == SDLK_l &&
                !(ev.key.keysym.mod & KMOD_CTRL)) {
                std::string lpath = dialog_load_path(hwnd);
                if (!lpath.empty()) {
                    std::ifstream lf(lpath);
                    if (lf) {
                        std::string js((std::istreambuf_iterator<char>(lf)),
                                       std::istreambuf_iterator<char>());
                        auto find_str = [&](const std::string& key, size_t from) -> std::string {
                            std::string pat = std::string(1,'"')+key+std::string("\":");
                            auto k = js.find(std::string(1,'"')+key+std::string("\":\""), from);
                            if (k == std::string::npos) return "";
                            k += key.size() + 4;
                            auto e2 = js.find('"', k);
                            return e2 == std::string::npos ? "" : js.substr(k, e2 - k);
                        };
                        auto find_int = [&](const std::string& key, size_t from) -> int {
                            auto k = js.find(std::string(1,'"')+key+std::string("\":"), from);
                            if (k == std::string::npos) return 0;
                            k += key.size() + 3;
                            try { return std::stoi(js.substr(k)); } catch (...) { return 0; }
                        };
                        auto mt = js.find("max_turns");
                        if (mt != std::string::npos) {
                            try { max_turns = std::stoi(js.substr(mt + 11)); } catch (...) {}
                            max_turns = std::clamp(max_turns, 4, 32);
                        }
                        std::vector<SetupEntry> loaded;
                        size_t pos = 0;
                        while ((pos = js.find("name", pos)) != std::string::npos) {
                            size_t obj = js.rfind("{", pos);
                            if (obj == std::string::npos) { ++pos; continue; }
                            SetupEntry e;
                            e.name       = find_str("name",       obj);
                            e.ship_class = find_str("ship_class", obj);
                            e.faction    = (Faction)find_int("faction",    obj);
                            e.included   = js.find("\"included\":true", obj) <
                                           js.find("{", obj + 1);
                            e.controller = (ShipController)find_int("controller", obj);
                            e.class_idx  = find_int("class_idx",  obj);
                            if (!e.name.empty()) loaded.push_back(e);
                            pos += 4;
                        }
                        if (!loaded.empty()) setup = std::move(loaded);
                    }
                }
                continue;
            }

            // â”€â”€ Setup phase â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€
            if (phase == GamePhase::Setup) {
                if (ev.type == SDL_MOUSEBUTTONDOWN && ev.button.button == SDL_BUTTON_LEFT) {
                    int mx = ev.button.x, my = ev.button.y;
                    for (auto& btn : setup_buttons) {
                        if (mx < btn.rect.x || mx > btn.rect.x + btn.rect.w) continue;
                        if (my < btn.rect.y || my > btn.rect.y + btn.rect.h) continue;
                        if (btn.action == 0 && btn.ship_idx >= 0) {
                            setup[btn.ship_idx].included = !setup[btn.ship_idx].included;
                        } else if (btn.action == 1 && btn.ship_idx >= 0) {
                            auto& c = setup[btn.ship_idx].controller;
                            c = (c == ShipController::Player1) ? ShipController::Player2
                              : (c == ShipController::Player2) ? ShipController::AI
                                                               : ShipController::Player1;
                        } else if (btn.action == 3 && btn.ship_idx >= 0) {
                            // Toggle dropdown for this row
                            if (setup_dropdown_row == btn.ship_idx) {
                                setup_dropdown_row = -1;
                            } else {
                                setup_dropdown_row    = btn.ship_idx;
                                setup_dropdown_scroll = 0;
                            }
                        } else if (btn.action == 4 && btn.ship_idx >= 0) {
                            // Select class from dropdown
                            auto& e = setup[btn.ship_idx];
                            const auto& cl = faction_classes(e.faction);
                            int ci = btn.value;
                            if (ci >= 0 && ci < (int)cl.size()) {
                                e.class_idx  = ci;
                                e.ship_class = cl[ci].first;
                            }
                            setup_dropdown_row = -1;
                        } else if (btn.action == 2) {
                            // Start Battle - need at least 2 included ships
                            int cnt = 0;
                            for (auto& e : setup) if (e.included) ++cnt;
                            if (cnt < 2) break;
                            phase = GamePhase::TerrainSetup;
                            board.for_each_mut([](Hex, Cell& cell){ cell.terrain = Terrain::Space; });
                        } else if (btn.action == 5) {
                            // Save -- push synthetic keypress so global S handler fires
                            SDL_Event sev{};
                            sev.type = SDL_KEYDOWN;
                            sev.key.keysym.sym = SDLK_s;
                            SDL_PushEvent(&sev);
                        } else if (btn.action == 6) {
                            SDL_Event lev{};
                            lev.type = SDL_KEYDOWN;
                            lev.key.keysym.sym = SDLK_l;
                            SDL_PushEvent(&lev);
                        } else if (btn.action == 7) {
                            running = false;
                        }
                        break;
                    }
                }
                // Close dropdown when clicking outside any action-3/4 button
                if (ev.type == SDL_MOUSEBUTTONDOWN && ev.button.button == SDL_BUTTON_LEFT
                    && setup_dropdown_row >= 0) {
                    bool hit = false;
                    for (auto& b2 : setup_buttons)
                        if ((b2.action == 3 || b2.action == 4) &&
                            ev.button.x >= b2.rect.x &&
                            ev.button.x <= b2.rect.x + b2.rect.w &&
                            ev.button.y >= b2.rect.y &&
                            ev.button.y <= b2.rect.y + b2.rect.h)
                        { hit = true; break; }
                    if (!hit) setup_dropdown_row = -1;
                }
                // Mousewheel scrolls open dropdown
                if (ev.type == SDL_MOUSEWHEEL && setup_dropdown_row >= 0) {
                    const auto& cl2 = faction_classes(setup[setup_dropdown_row].faction);
                    int total2 = (int)cl2.size();
                    int max_sc = (total2 > 14) ? total2 - 14 : 0;
                    setup_dropdown_scroll -= ev.wheel.y;
                    if (setup_dropdown_scroll < 0)      setup_dropdown_scroll = 0;
                    if (setup_dropdown_scroll > max_sc) setup_dropdown_scroll = max_sc;
                }
                // [ / ] adjust turn count; S = save scenario; L = load scenario
                if (ev.type == SDL_KEYDOWN) {
                    if (ev.key.keysym.sym == SDLK_ESCAPE)
                        setup_dropdown_row = -1;
                    else if (ev.key.keysym.sym == SDLK_LEFTBRACKET)
                        max_turns = std::max(4, max_turns - 2);
                    else if (ev.key.keysym.sym == SDLK_RIGHTBRACKET)
                        max_turns = std::min(32, max_turns + 2);
                    else if (ev.key.keysym.sym == SDLK_s) {
                        // Save scenario via Windows dialog
                        std::string spath = dialog_save_path(hwnd);
                        if (!spath.empty()) {
                            std::ofstream sf(spath);
                            if (sf) {
                                sf << "{\"max_turns\":" << max_turns << ",\"ships\":[";
                                for (int i = 0; i < (int)setup.size(); ++i) {
                                    const auto& e = setup[i];
                                    if (i) sf << ",";
                                    sf << "{\"name\":\"" << e.name << "\""
                                       << ",\"ship_class\":\"" << e.ship_class << "\""
                                       << ",\"faction\":" << (int)e.faction
                                       << ",\"included\":" << (e.included ? "true" : "false")
                                       << ",\"controller\":" << (int)e.controller
                                       << ",\"class_idx\":" << e.class_idx << "}";
                                }
                                sf << "]}";
                            }
                        }
                    } else if (ev.key.keysym.sym == SDLK_l) {
                        // Load scenario via Windows dialog
                        std::string lpath = dialog_load_path(hwnd);
                        if (!lpath.empty()) {
                        std::ifstream lf(lpath);
                        if (lf) {
                            std::string js((std::istreambuf_iterator<char>(lf)),
                                           std::istreambuf_iterator<char>());
                            // Parse max_turns
                            auto mt = js.find("\"max_turns\":");
                            if (mt != std::string::npos) {
                                try { max_turns = std::stoi(js.substr(mt + 12)); } catch (...) {}
                                max_turns = std::clamp(max_turns, 4, 32);
                            }
                            // Parse each ship entry
                            std::vector<SetupEntry> loaded;
                            size_t pos = 0;
                            while ((pos = js.find("{\"name\":", pos)) != std::string::npos) {
                                auto parse_str = [&](const std::string& key) -> std::string {
                                    auto k = js.find("\"" + key + "\":\"", pos);
                                    if (k == std::string::npos) return "";
                                    k += key.size() + 4;
                                    auto e = js.find('"', k);
                                    return e == std::string::npos ? "" : js.substr(k, e - k);
                                };
                                auto parse_int = [&](const std::string& key) -> int {
                                    auto k = js.find("\"" + key + "\":", pos);
                                    if (k == std::string::npos) return 0;
                                    k += key.size() + 3;
                                    try { return std::stoi(js.substr(k)); } catch (...) { return 0; }
                                };
                                SetupEntry e;
                                e.name       = parse_str("name");
                                e.ship_class = parse_str("ship_class");
                                e.faction    = (Faction)parse_int("faction");
                                e.included   = js.find("\"included\":true", pos) < js.find('{', pos + 1);
                                e.controller = (ShipController)parse_int("controller");
                                e.class_idx  = parse_int("class_idx");
                                if (!e.name.empty()) loaded.push_back(e);
                                ++pos;
                            }
                            if (!loaded.empty()) setup = std::move(loaded);
                        }
                    }
                    }
                }
                continue; // don't process other phases during setup
            }

            // â”€â”€ Game over â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€
            if (phase == GamePhase::GameOver) {
                if (ev.type == SDL_KEYDOWN) {
                    if (ev.key.keysym.sym == SDLK_r) {
                        phase = GamePhase::Setup;
                        setup = default_setup();
                        show_ssd = false;
                    }
                    if (ev.key.keysym.sym == SDLK_ESCAPE) running = false;
                }
                continue;
            }

            // â”€â”€ Planning phase â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€
            if (phase == GamePhase::TerrainSetup) {
                if (ev.type == SDL_KEYDOWN) {
                    switch (ev.key.keysym.sym) {
                    case SDLK_1: terrain_brush = 0; break;
                    case SDLK_2: terrain_brush = 1; break;
                    case SDLK_3: terrain_brush = 2; break;
                    case SDLK_4: terrain_brush = 3; break;
                    case SDLK_5: terrain_brush = 4; break;  // BlackHole
                    case SDLK_6: terrain_brush = 5; break;  // DustCloud
                    case SDLK_7: terrain_brush = 6; break;  // IonStorm
                    case SDLK_8: terrain_brush = 7; break;  // GravityRift
                    case SDLK_9: terrain_brush = 8; break;  // TholianWeb
                    case SDLK_r: rift_dir_brush = (rift_dir_brush + 1) % 6; break;  // cycle rift dir
                    case SDLK_RETURN: case SDLK_KP_ENTER:
                        phase = GamePhase::Placement;
                        ships.clear();
                        { int sl=0; for(auto& e:setup) if(e.included) ships.push_back(build_ship(e,sl++)); }
                        for(auto& s:ships) if(s.controller==ShipController::AI) s.pos_confirmed=true;
                        placement_idx=0;
                        while(placement_idx<(int)ships.size()&&ships[placement_idx].controller==ShipController::AI) ++placement_idx;
                        break;
                    default: break;
                    }
                }
                if (ev.type == SDL_MOUSEBUTTONDOWN && ev.button.button == SDL_BUTTON_LEFT) {
                    int tx=ev.button.x, ty=ev.button.y;
                    int bx=W-SIDEBAR_W+10;
                    if (tx < W-SIDEBAR_W) {
                        SDL_FPoint tpt={(float)tx,(float)ty};
                        Hex th=rend.pixel_to_hex(tpt);
                        if (board.contains(th)) {
                            static const Terrain tbrushes[]={Terrain::Space,Terrain::Nebula,Terrain::Asteroid,Terrain::Planet,Terrain::BlackHole,Terrain::DustCloud,Terrain::IonStorm,Terrain::GravityRift,Terrain::TholianWeb};
                            board.at(th).terrain=tbrushes[terrain_brush];
                            if (tbrushes[terrain_brush]==Terrain::GravityRift)
                                board.at(th).rift_dir=rift_dir_brush;
                        }
                    } else {
                        for(int bi=0;bi<9;++bi){int by=60+bi*34;if(tx>=bx&&tx<=bx+240&&ty>=by&&ty<=by+28)terrain_brush=bi;}
                        int done_y=60+9*34+20;
                        if(tx>=bx&&tx<=bx+240&&ty>=done_y&&ty<=done_y+32){
                            phase=GamePhase::Placement;
                            ships.clear();{int sl=0;for(auto& e:setup)if(e.included)ships.push_back(build_ship(e,sl++));}
                            for(auto& s:ships)if(s.controller==ShipController::AI)s.pos_confirmed=true;
                            placement_idx=0;
                            while(placement_idx<(int)ships.size()&&ships[placement_idx].controller==ShipController::AI)++placement_idx;
                        }
                    }
                }
                continue;
            }

            if (phase == GamePhase::Placement) {
                if (ev.type == SDL_MOUSEBUTTONDOWN && ev.button.button == SDL_BUTTON_LEFT) {
                    int pmx=ev.button.x,pmy=ev.button.y;
                    if (pmx<board_area_w&&placement_idx<(int)ships.size()) {
                        SDL_FPoint ppt={(float)pmx,(float)pmy};
                        Hex ph=rend.pixel_to_hex(ppt);
                        if(board.contains(ph)){
                            Terrain pt=board.at(ph).terrain;
                            bool blocked=(pt==Terrain::Asteroid||pt==Terrain::Planet||
                                          pt==Terrain::TholianWeb||pt==Terrain::BlackHole);
                            for(auto& s:ships)if(s.pos_confirmed&&s.position.q==ph.q&&s.position.r==ph.r)blocked=true;
                            if(!blocked){
                                ships[placement_idx].position=ph;
                                ships[placement_idx].pos_confirmed=true;
                                ++placement_idx;
                                while(placement_idx<(int)ships.size()&&ships[placement_idx].controller==ShipController::AI)++placement_idx;
                                if(placement_idx>=(int)ships.size()){
                                    phase=GamePhase::Planning;eaf_player=0;eaf_scroll=0;
                                    turn=1;impulse=1;selected=-1;fire_mode=false;winner.clear();
                                    exp_ship=-1;exp_frame=0;msg_buf[0]=0;msg_ttl=0;combat_log.clear();
                                    llm_plans.assign(ships.size(),LlmPlan{});
                                    crew_advices.assign(ships.size(),CrewAdvice{});
                                    g_seekers.clear();
                                    advance_eaf_player(eaf_player,ships,phase,impulse,llm_plans,turn,max_turns,llm_on,combat_log);
                                    if(llm_on&&eaf_player<(int)ships.size()&&ships[eaf_player].controller!=ShipController::AI){
                                        ensure_crew_names(ships[eaf_player]);
                                        crew_advices[eaf_player]=call_crew_llm(ships[eaf_player],ships,turn,max_turns);
                                        if(crew_advices[eaf_player].valid){const auto& a=crew_advices[eaf_player];
                                            combat_log.push_back("[BRIDGE] "+a.helm);
                                            combat_log.push_back("[BRIDGE] "+a.weapons);
                                            combat_log.push_back("[BRIDGE] "+a.engineering);
                                            combat_log.push_back("[BRIDGE] "+a.science);
                                            while(combat_log.size()>40)combat_log.pop_front();}
                                    }
                                }
                            }
                        }
                    }
                }
                continue;
            }
            if (phase == GamePhase::Planning) {
                if (ev.type == SDL_MOUSEBUTTONDOWN && ev.button.button == SDL_BUTTON_LEFT) {
                    int mx = ev.button.x, my = ev.button.y;
                    for (auto& btn : eaf_buttons) {
                        if (mx < btn.rect.x || mx > btn.rect.x + btn.rect.w) continue;
                        if (my < btn.rect.y || my > btn.rect.y + btn.rect.h) continue;

                        Ship& s = ships[eaf_player];
                        switch (btn.field) {
                        case EafField::WarpPower:
                            s.eaf.warp_power = std::clamp(s.eaf.warp_power + btn.delta, 0, s.sys.max_warp_power);
                            break;
                        case EafField::ImpulsePower:
                            s.eaf.impulse_power = std::clamp(s.eaf.impulse_power + btn.delta, 0, s.sys.max_impulse_power);
                            break;
                        case EafField::AprOutput:
                            s.eaf.apr_output = std::clamp(s.eaf.apr_output + btn.delta, 0, s.sys.apr_current);
                            break;
                        case EafField::BatteryTap:
                            s.eaf.battery_tap = std::clamp(s.eaf.battery_tap + btn.delta, 0, s.sys.battery_charge);
                            break;
                        case EafField::Cloak:
                            if (s.sys.cloak_installed)
                                s.eaf.cloak = (s.eaf.cloak > 0) ? 0 : s.sys.cloak_cost;
                            break;
                        case EafField::EngineDouble:
                            if (s.faction == Faction::Orion)
                                s.eaf.engine_double = !s.eaf.engine_double;
                            break;
                        case EafField::Speed: {
                            // (C2.21) Accel cap: new speed <= last_speed + max(last_speed, 10)
                            // (C8.2) Emergency decel restriction: force speed 0
                            if (s.emerg_decel_impulses > 0) break; // blocked during restriction
                            int accel_max = std::min(31, s.last_speed + std::max(s.last_speed, 10));
                            int mv_cap    = std::min(accel_max, s.movement_power());
                            s.eaf.speed = std::clamp(s.eaf.speed + btn.delta, 0, mv_cap);
                            break;
                        }
                        case EafField::Shield:
                            s.eaf.reinforce[btn.index] = std::clamp(s.eaf.reinforce[btn.index] + btn.delta, 0, s.sys.shields[btn.index]);
                            break;
                        case EafField::GeneralReinforce:
                            s.eaf.general_reinforce = std::clamp(s.eaf.general_reinforce + btn.delta, 0, 8);
                            break;
                        case EafField::Ecm:
                            s.eaf.ecm = std::clamp(s.eaf.ecm + btn.delta, 0, 4);
                            break;
                        case EafField::Eccm:
                            s.eaf.eccm = std::clamp(s.eaf.eccm + btn.delta, 0, 4);
                            break;
                        case EafField::Repair:
                            s.eaf.repair = std::clamp(s.eaf.repair + btn.delta * 2, 0, 16);
                            break;
                        case EafField::Lab:
                            s.eaf.lab = std::clamp(s.eaf.lab + btn.delta, 0, s.sys.max_lab);
                            break;
                        case EafField::Tractors:
                            s.eaf.tractors = std::clamp(s.eaf.tractors + btn.delta, 0, 4);
                            break;
                        case EafField::Transporters:
                            s.eaf.transporters = std::clamp(s.eaf.transporters + btn.delta, 0, 2);
                            break;
                        case EafField::WeaponAlloc: {
                            Weapon& w = s.weapons[btn.index];
                            if (!w.disabled) w.allocated = std::clamp(w.allocated + btn.delta, 0, w.max_power);
                            break;
                        }
                        case EafField::WeaponArm: {
                            Weapon& w = s.weapons[btn.index];
                            if (!w.disabled) {
                                if (w.armed)          { w.armed = false; w.charge = 0; w.allocated = 0; }
                                else if (w.allocated) { w.allocated = 0; }
                                else                  { w.allocated = w.arming_cost; }
                            }
                            break;
                        }
                        case EafField::Commit:
                            commit_ship_eaf(s);
                            ++eaf_player;
                            eaf_scroll = 0;
                            advance_eaf_player(eaf_player, ships, phase, impulse,
                                               llm_plans, turn, max_turns, llm_on, combat_log);
                            // Crew advice for next player ship
                            if (llm_on && phase == GamePhase::Planning &&
                                eaf_player < (int)ships.size() &&
                                ships[eaf_player].controller != ShipController::AI &&
                                (int)crew_advices.size() > eaf_player) {
                                ensure_crew_names(ships[eaf_player]);
                                crew_advices[eaf_player] = call_crew_llm(
                                    ships[eaf_player], ships, turn, max_turns);
                                if (crew_advices[eaf_player].valid) {
                                    const auto& a = crew_advices[eaf_player];
                                    combat_log.push_back("[BRIDGE] " + a.helm);
                                    combat_log.push_back("[BRIDGE] " + a.weapons);
                                    combat_log.push_back("[BRIDGE] " + a.engineering);
                                    combat_log.push_back("[BRIDGE] " + a.science);
                                    while (combat_log.size() > 40) combat_log.pop_front();
                                }
                            }
                            break;
                        }
                        break;
                    }
                }
                if (ev.type == SDL_KEYDOWN && ev.key.keysym.sym == SDLK_TAB)
                    eaf_hidden = !eaf_hidden;
                if (ev.type == SDL_MOUSEWHEEL) {
                    eaf_scroll = std::clamp(eaf_scroll + ev.wheel.y * -20, 0, 600);
                }
            }
            if (ev.type == SDL_MOUSEWHEEL && (SDL_GetModState() & KMOD_CTRL)) {
                hex_size = std::clamp(hex_size + ev.wheel.y * 3.0f, 18.0f, 90.0f);
                rend.set_hex_size(hex_size);
            }
            if (ev.type == SDL_KEYDOWN &&
                (ev.key.keysym.sym == SDLK_EQUALS || ev.key.keysym.sym == SDLK_KP_PLUS)) {
                hex_size = std::clamp(hex_size + 5.0f, 18.0f, 90.0f); rend.set_hex_size(hex_size);
            }
            if (ev.type == SDL_KEYDOWN && ev.key.keysym.sym == SDLK_MINUS) {
                hex_size = std::clamp(hex_size - 5.0f, 18.0f, 90.0f); rend.set_hex_size(hex_size);
            }
                // Crew name editing: click a row, type, Enter confirms
                if (ev.type == SDL_MOUSEBUTTONDOWN && ev.button.button == SDL_BUTTON_LEFT
                    && eaf_player < (int)ships.size()
                    && ships[eaf_player].controller != ShipController::AI) {
                    int mx = ev.button.x, my = ev.button.y;
                    for (int ri = 0; ri < (int)crew_row_rects.size(); ++ri) {
                        const auto& r = crew_row_rects[ri];
                        if (mx >= r.x && mx < r.x + r.w && my >= r.y && my < r.y + r.h) {
                            if (crew_edit_role == ri) {
                                crew_edit_role = -1; crew_edit_buf.clear(); SDL_StopTextInput();
                            } else {
                                crew_edit_role = ri; crew_edit_buf.clear(); SDL_StartTextInput();
                            }
                            break;
                        }
                    }
                }
                if (crew_edit_role >= 0) {
                    if (ev.type == SDL_TEXTINPUT) {
                        crew_edit_buf += ev.text.text;
                    } else if (ev.type == SDL_KEYDOWN) {
                        auto sym = ev.key.keysym.sym;
                        if (sym == SDLK_BACKSPACE && !crew_edit_buf.empty())
                            crew_edit_buf.pop_back();
                        else if (sym == SDLK_RETURN || sym == SDLK_KP_ENTER) {
                            if (!crew_edit_buf.empty() && eaf_player < (int)crew_advices.size()) {
                                auto& ca = crew_advices[eaf_player];
                                std::string* flds[4] = {&ca.helm, &ca.weapons, &ca.engineering, &ca.science};
                                auto col = flds[crew_edit_role]->find(':');
                                if (col != std::string::npos)
                                    *flds[crew_edit_role] = flds[crew_edit_role]->substr(0, col + 2) + crew_edit_buf;
                                else
                                    *flds[crew_edit_role] = crew_edit_buf;
                                update_crew_name(ships[eaf_player].name, crew_edit_role, crew_edit_buf);
                                if (g_crew_roster.count(ships[eaf_player].name))
                                    g_crew_roster[ships[eaf_player].name][crew_edit_role] = crew_edit_buf;
                            }
                            crew_edit_role = -1; crew_edit_buf.clear(); SDL_StopTextInput();
                        } else if (sym == SDLK_ESCAPE) {
                            crew_edit_role = -1; crew_edit_buf.clear(); SDL_StopTextInput();
                        }
                    }
                }

            // â”€â”€ Impulse phase â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€
            else if (phase == GamePhase::Impulse) {
                // Pause menu: handle button clicks, swallow all other input
                if (pause_menu_open) {
                    if (ev.type == SDL_MOUSEBUTTONDOWN && ev.button.button == SDL_BUTTON_LEFT) {
                        int pmx = ev.button.x, pmy = ev.button.y;
                        for (auto& pb : pause_buttons) {
                            if (pmx < pb.rect.x || pmx > pb.rect.x + pb.rect.w) continue;
                            if (pmy < pb.rect.y || pmy > pb.rect.y + pb.rect.h) continue;
                            switch (pb.action) {
                            case 0: pause_menu_open = false; break;
                            case 1: { // Save scenario
                                SDL_Event sev{};
                                sev.type = SDL_KEYDOWN;
                                sev.key.keysym.sym = SDLK_s;
                                SDL_PushEvent(&sev);
                                pause_menu_open = false;
                                break;
                            }
                            case 2: { // Load scenario
                                SDL_Event lev{};
                                lev.type = SDL_KEYDOWN;
                                lev.key.keysym.sym = SDLK_l;
                                SDL_PushEvent(&lev);
                                pause_menu_open = false;
                                break;
                            }
                            case 3: // Return to setup
                                phase = GamePhase::Setup;
                                ships.clear();
                                pause_menu_open = false;
                                break;
                            case 4: running = false; break;
                            }
                            break;
                        }
                    }
                    if (ev.type != SDL_KEYDOWN) continue;
                }
                if (ev.type == SDL_KEYDOWN) {
                    switch (ev.key.keysym.sym) {
                    case SDLK_BACKSPACE: {
                        // (C8.0) Emergency Deceleration: immediate stop this turn
                        if (selected >= 0 && !ships[selected].destroyed) {
                            Ship& es = ships[selected];
                            if (es.move_available && es.emerg_decel_impulses == 0) {
                                // Convert unused movement to forward shield (C8.11)
                                int unused_mv = es.eaf.speed - es.moves_used;
                                if (unused_mv > 0) {
                                    es.sys.shields[0] = std::min(
                                        es.sys.shields_max[0],
                                        es.sys.shields[0] + unused_mv / 2);
                                }
                                es.eaf.speed  = 0;
                                es.move_available = false;
                                es.emerg_decel_impulses = 16; // (C8.2) restriction period
                                std::snprintf(msg_buf, sizeof(msg_buf),
                                    "EMERGENCY DECEL! %s stops. 16-impulse restriction.", es.name.c_str());
                                msg_ttl = 180;
                            }
                        }
                        break;
                    }
                    case SDLK_q:
                        if (selected >= 0 && !ships[selected].destroyed) {
                            Ship& sel = ships[selected];
                            bool het = (ev.key.keysym.mod & KMOD_SHIFT) != 0;
                            int tm_q = turn_mode(sel.eaf.speed, sel.turn_mode_cat);
                            bool can_q = (sel.hexes_since_turn >= tm_q);
                            if (het && !sel.het_used && sel.sys.battery_charge >= 5
                                && sel.emerg_decel_impulses == 0) { // (C8.2) no HET in restriction
                                sel.facing = (sel.facing + 4) % 6;
                                sel.sys.battery_charge -= 5;
                                sel.het_used = true;
                                sel.hexes_since_turn = 0;
                                std::snprintf(msg_buf, sizeof(msg_buf),
                                    "HET! %s -2 facing (battery -5)", sel.name.c_str());
                                msg_ttl = 120; push_log();
                                // C6.51 breakdown: roll d6; on 6 warp engine takes 3-pt DAC hit
                                if ((rand() % 6) + 1 == 6) {
                                    sel.apply_damage(3, 0);
                                    std::snprintf(msg_buf, sizeof(msg_buf),
                                        "HET BREAKDOWN! %s warp engine hit!", sel.name.c_str());
                                    msg_ttl = 180; push_log();
                                }
                            } else if (!het && can_q) {
                                sel.facing = (sel.facing + 5) % 6;
                                sel.hexes_since_turn = 0;
                            } else if (!het) {
                                int need_q = tm_q - sel.hexes_since_turn;
                                std::snprintf(msg_buf, sizeof(msg_buf),
                                    "TM %c: need %d more hex(es) before turning.", 'A'+tm_q-1, need_q);
                                msg_ttl = 90;
                            }
                        }
                        break;
                    case SDLK_e:
                        if (selected >= 0 && !ships[selected].destroyed) {
                            Ship& sel = ships[selected];
                            bool het = (ev.key.keysym.mod & KMOD_SHIFT) != 0;
                            int tm_e = turn_mode(sel.eaf.speed, sel.turn_mode_cat);
                            bool can_e = (sel.hexes_since_turn >= tm_e);
                            if (het && !sel.het_used && sel.sys.battery_charge >= 5) {
                                sel.facing = (sel.facing + 2) % 6;
                                sel.sys.battery_charge -= 5;
                                sel.het_used = true;
                                sel.hexes_since_turn = 0;
                                std::snprintf(msg_buf, sizeof(msg_buf),
                                    "HET! %s +2 facing (battery -5)", sel.name.c_str());
                                msg_ttl = 120; push_log();
                                // C6.51 breakdown: roll d6; on 6 warp engine takes 3-pt DAC hit
                                if ((rand() % 6) + 1 == 6) {
                                    sel.apply_damage(3, 0);
                                    std::snprintf(msg_buf, sizeof(msg_buf),
                                        "HET BREAKDOWN! %s warp engine hit!", sel.name.c_str());
                                    msg_ttl = 180; push_log();
                                }
                            } else if (!het && can_e) {
                                sel.facing = (sel.facing + 1) % 6;
                                sel.hexes_since_turn = 0;
                            } else if (!het) {
                                int need_e = tm_e - sel.hexes_since_turn;
                                std::snprintf(msg_buf, sizeof(msg_buf),
                                    "TM %c: need %d more hex(es) before turning.", 'A'+tm_e-1, need_e);
                                msg_ttl = 90;
                            }
                        }
                        break;
                    case SDLK_TAB:
                        show_ssd = !show_ssd;
                        break;
                    case SDLK_l:
                        llm_on = !llm_on;
                        std::snprintf(msg_buf, sizeof(msg_buf),
                            "Bridge AI: %s", llm_on ? "ONLINE" : "OFFLINE (deterministic)");
                        msg_ttl = 180; push_log();
                        break;
                    case SDLK_b:
                        // H7.0: reactive battery tap
                        if (selected >= 0 && !ships[selected].destroyed) {
                            Ship& bs = ships[selected];
                            if (bs.sys.battery_charge >= 2) {
                                bs.sys.battery_charge -= 2;
                                bs.eaf.reinforce[0] = std::min(bs.eaf.reinforce[0] + 1, bs.sys.shields[0]);
                                std::snprintf(msg_buf, sizeof(msg_buf),
                                    "%s: reactive battery (-2) -> fwd shield +1 [H7.0]", bs.name.c_str());
                                msg_ttl = 120; push_log();
                            } else {
                                std::snprintf(msg_buf, sizeof(msg_buf),
                                    "%s: no battery for reactive tap.", bs.name.c_str());
                                msg_ttl = 90;
                            }
                        }
                        break;
                    case SDLK_a:  // C4.0: sideslip left
                    case SDLK_d:  // C4.0: sideslip right
                        if (selected >= 0 && !ships[selected].destroyed) {
                            Ship& sel = ships[selected];
                            if (sel.move_available) {
                                int tm_ss = turn_mode(sel.eaf.speed, sel.turn_mode_cat);
                                if (sel.hexes_since_turn >= tm_ss) {
                                    // sideslip-left = facing-1 dir; sideslip-right = facing+1
                                    int ss_dir = (ev.key.keysym.sym == SDLK_a)
                                        ? (sel.facing + 5) % 6
                                        : (sel.facing + 1) % 6;
                                    Hex ss_hex = hex_neighbour(sel.position, ss_dir);
                                    bool bad_t = !board.contains(ss_hex) ||
                                        board.at(ss_hex).terrain == Terrain::Asteroid ||
                                        board.at(ss_hex).terrain == Terrain::Planet;
                                    bool occ = false;
                                    for (auto& oo : ships)
                                        if (!oo.destroyed && &oo != &sel && oo.position == ss_hex)
                                            { occ = true; break; }
                                    if (!bad_t && !occ) {
                                        sel.position = ss_hex;
                                        sel.move_available = false;
                                        ++sel.moves_used;
                                        sel.hexes_since_turn = 0;
                                        std::snprintf(msg_buf, sizeof(msg_buf),
                                            "%s sideslips %s [C4.0]",
                                            sel.name.c_str(),
                                            ev.key.keysym.sym == SDLK_a ? "left" : "right");
                                        msg_ttl = 90;
                                    } else {
                                        std::snprintf(msg_buf, sizeof(msg_buf),
                                            "Sideslip blocked (impassable or occupied).");
                                        msg_ttl = 90;
                                    }
                                } else {
                                    int need_ss = tm_ss - sel.hexes_since_turn;
                                    std::snprintf(msg_buf, sizeof(msg_buf),
                                        "TM %c: need %d more hex(es) before sideslip.",
                                        'A'+tm_ss-1, need_ss);
                                    msg_ttl = 90;
                                }
                            }
                        }
                        break;
                    case SDLK_f:
                        fire_mode = (selected >= 0 && !ships[selected].destroyed) ? !fire_mode : false;
                        if (!fire_mode) selected_weapon = -1;
                        break;
                    case SDLK_ESCAPE:
                        if (pause_menu_open)  { pause_menu_open = false; }
                        else if (show_ssd)    { show_ssd = false; }
                        else if (fire_mode)   { fire_mode = false; selected_weapon = -1; }
                        else if (selected >= 0) { selected = -1; }
                        else                  { pause_menu_open = true; }
                        break;
                    case SDLK_F1:
                        show_ssd = (selected >= 0) ? !show_ssd : false;
                        break;
                    case SDLK_SPACE: {
                        // Block advance if any human ship can still legally move forward
                        bool pending_move = false;
                        for (auto& sp_s : ships) {
                            if (sp_s.destroyed || sp_s.controller == ShipController::AI) continue;
                            if (!sp_s.move_available) continue;
                            Hex sp_fwd = forward_hex(sp_s);
                            if (!board.contains(sp_fwd)) continue;
                            Terrain sp_t = board.at(sp_fwd).terrain;
                            if (sp_t == Terrain::Asteroid || sp_t == Terrain::Planet) continue;
                            bool sp_occ = false;
                            for (auto& oo : ships)
                                if (!oo.destroyed && &oo != &sp_s &&
                                    oo.position.q == sp_fwd.q && oo.position.r == sp_fwd.r)
                                    { sp_occ = true; break; }
                            if (sp_occ) continue;
                            pending_move = true; break;
                        }
                        if (pending_move) {
                            std::snprintf(msg_buf, sizeof(msg_buf),
                                "Move your ships forward (Q/E to turn, A/D to sideslip) before advancing.");
                            msg_ttl = 150;
                            break;
                        }
                        selected_weapon = -1; fire_mode = false;

                        // AI acts at end of each impulse
                        run_ai_impulse(ships, board, rend, impulse,
                                       msg_buf, msg_ttl, exp_ship, exp_frame, llm_plans,
                                       g_seekers, combat_log);
                        if (msg_ttl > 0) push_log();

                        // Advance seeking weapons
                        advance_seekers(g_seekers, ships, msg_buf, msg_ttl,
                                        exp_ship, exp_frame, combat_log);
                        if (exp_ship >= 0 && !ships[exp_ship].exploded)
                            apply_explosion(ships[exp_ship], ships, msg_buf, msg_ttl);

                        // WYN radiation zone (P7): each WYN ship deals 1 damage to all
                        // enemy ships within 2 hexes every impulse
                        for (auto& wyn : ships) {
                            if (wyn.destroyed) continue;
                            if (!wyn.ship_class.starts_with("wyn_")) continue;
                            for (auto& enemy : ships) {
                                if (enemy.destroyed || enemy.faction == wyn.faction) continue;
                                int r = wyn.position.distance(enemy.position);
                                if (r <= 2) {
                                    int sh = hit_facing(wyn, enemy);
                                    enemy.apply_damage(1, sh);
                                }
                            }
                        }

                        // Advanced terrain effects (end of each impulse)
                        // P8 Black Hole: pull all ships within 3 hexes one step inward
                        {
                            std::vector<Hex> bh_hexes;
                            board.for_each([&](Hex h, const Cell& c){
                                if (c.terrain == Terrain::BlackHole) bh_hexes.push_back(h);
                            });
                            for (auto& bh : bh_hexes) {
                                for (auto& ship : ships) {
                                    if (ship.destroyed) continue;
                                    int dist = ship.position.distance(bh);
                                    if (dist == 0) {
                                        ship.sys.hull = 0; ship.destroyed = true;
                                        std::snprintf(msg_buf, sizeof(msg_buf),
                                            "%s: consumed by black hole!", ship.name.c_str());
                                        msg_ttl = 240; push_log();
                                    } else if (dist <= 3) {
                                        int best = -1, bd = dist;
                                        for (int d = 0; d < 6; ++d) {
                                            Hex nb = hex_neighbour(ship.position, d);
                                            int nd = nb.distance(bh);
                                            if (nd < bd) { bd = nd; best = d; }
                                        }
                                        if (best >= 0) {
                                            Hex ph2 = hex_neighbour(ship.position, best);
                                            bool occ2 = false;
                                            for (auto& s2 : ships)
                                                if (!s2.destroyed && &s2 != &ship && s2.position == ph2)
                                                    { occ2 = true; break; }
                                            if (!occ2 && board.contains(ph2))
                                                ship.position = ph2;
                                        }
                                    }
                                }
                            }
                        }
                        // P9 Dust Cloud: (fire penalty applied inline in fire resolution)
                        // P10 Ion Storm: drain 1 battery per impulse; hull dmg if empty
                        for (auto& ship : ships) {
                            if (ship.destroyed) continue;
                            if (board.at(ship.position).terrain == Terrain::IonStorm) {
                                if (ship.sys.battery_charge > 0)
                                    --ship.sys.battery_charge;
                                else {
                                    ship.apply_damage(1, 0);
                                    std::snprintf(msg_buf, sizeof(msg_buf),
                                        "%s: ion storm disruption! (hull hit)", ship.name.c_str());
                                    msg_ttl = 120; push_log();
                                }
                            }
                        }
                        // P11 Gravity Rift: push ship one hex in rift direction
                        for (auto& ship : ships) {
                            if (ship.destroyed) continue;
                            const Cell& rcell = board.at(ship.position);
                            if (rcell.terrain == Terrain::GravityRift) {
                                Hex rh = hex_neighbour(ship.position, rcell.rift_dir % 6);
                                if (!board.contains(rh)) continue;
                                Terrain rt = board.at(rh).terrain;
                                if (rt == Terrain::Asteroid || rt == Terrain::Planet ||
                                    rt == Terrain::TholianWeb || rt == Terrain::BlackHole) {
                                    ship.apply_damage(2, 0);  // collision
                                    std::snprintf(msg_buf, sizeof(msg_buf),
                                        "%s: rift collision! (2 hull dmg)", ship.name.c_str());
                                    msg_ttl = 120; push_log();
                                } else {
                                    bool occ3 = false;
                                    for (auto& s3 : ships)
                                        if (!s3.destroyed && &s3 != &ship && s3.position == rh)
                                            { occ3 = true; break; }
                                    if (!occ3) {
                                        ship.position = rh;
                                        ++ship.hexes_since_turn;
                                    }
                                }
                            }
                        }

                        winner = check_victory(ships);
                        if (!winner.empty()) { phase = GamePhase::GameOver; break; }

                        if (impulse < 32) {
                            ++impulse;
                            for (auto& ship : ships) {
                                if (ship.destroyed) continue;
                                // (C8.2) Emergency decel restriction period countdown
                                if (ship.emerg_decel_impulses > 0) {
                                    --ship.emerg_decel_impulses;
                                    ship.move_available = false; // stopped
                                } else {
                                    ship.move_available = moves_this_impulse(ship.eaf.speed, impulse);
                                }
                            }
                        } else {
                            ++turn;
                            // Turn limit: winner is whoever has more hull remaining
                            if (turn > max_turns) {
                                int best_hull = -1;
                                winner = "DRAW";
                                for (auto& s : ships) {
                                    if (s.destroyed) continue;
                                    int pct = s.sys.hull * 100 / std::max(1, s.sys.hull_max);
                                    if (pct > best_hull) { best_hull = pct; winner = s.name; }
                                }
                                phase = GamePhase::GameOver;
                                break;
                            }
                            recall_fighters(g_seekers, ships); // fighters return to bay before planning
                            // Drone endurance: decrement turn counter, expire at 0
                            for (auto& sk : g_seekers) {
                                if (sk.turns_remaining > 0) {
                                    --sk.turns_remaining;
                                    if (sk.turns_remaining == 0) sk.spent = true;
                                }
                            }
                            g_seekers.erase(std::remove_if(g_seekers.begin(), g_seekers.end(),
                                [](const SeekingWeapon& s){ return s.spent; }), g_seekers.end());
                            phase = GamePhase::Planning;
                            eaf_player = 0; eaf_scroll = 0;
                            selected = -1;
                            for (auto& ship : ships) if (!ship.destroyed) ship.begin_planning();
                            llm_plans.assign(ships.size(), LlmPlan{});
                            crew_advices.assign(ships.size(), CrewAdvice{});
                            advance_eaf_player(eaf_player, ships, phase, impulse,
                                               llm_plans, turn, max_turns, llm_on, combat_log);
                            // Crew advice for first player ship of new turn
                            if (llm_on && eaf_player < (int)ships.size() &&
                                ships[eaf_player].controller != ShipController::AI) {
                                ensure_crew_names(ships[eaf_player]);
                                crew_advices[eaf_player] = call_crew_llm(
                                    ships[eaf_player], ships, turn, max_turns);
                                if (crew_advices[eaf_player].valid) {
                                    const auto& a = crew_advices[eaf_player];
                                    combat_log.push_back("[BRIDGE] " + a.helm);
                                    combat_log.push_back("[BRIDGE] " + a.weapons);
                                    combat_log.push_back("[BRIDGE] " + a.engineering);
                                    combat_log.push_back("[BRIDGE] " + a.science);
                                    while (combat_log.size() > 40) combat_log.pop_front();
                                }
                            }
                        }
                        break;
                    }
                    }
                }

                if (ev.type == SDL_MOUSEBUTTONDOWN && ev.button.button == SDL_BUTTON_LEFT) {
                    int mx = ev.button.x, my = ev.button.y;

                    // Sidebar weapon buttons
                    bool sidebar_hit = false;
                    for (auto& btn : weapon_buttons) {
                        if (mx < btn.rect.x || mx > btn.rect.x + btn.rect.w) continue;
                        if (my < btn.rect.y || my > btn.rect.y + btn.rect.h) continue;
                        selected_weapon = (btn.index == selected_weapon) ? -1 : btn.index;
                        fire_mode = (selected_weapon >= 0);
                        sidebar_hit = true; break;
                    }
                    if (sidebar_hit) break;
                    if (mx >= board_area_w) break;

                    SDL_FPoint mp = {(float)mx, (float)my};
                    Hex clicked = rend.pixel_to_hex(mp);
                    if (!board.contains(clicked)) break;

                    int hit = -1;
                    for (int i = 0; i < (int)ships.size(); ++i)
                        if (!ships[i].destroyed &&
                            ships[i].position.q == clicked.q &&
                            ships[i].position.r == clicked.r) { hit = i; break; }

                    if (fire_mode && selected >= 0 && selected_weapon >= 0) {
                        if (hit >= 0 && hit != selected) {
                            Ship& atk = ships[selected];
                            Ship& tgt = ships[hit];
                            Weapon& w  = atk.weapons[selected_weapon];

                            // Cloaked ships cannot fire (would reveal position)
                            if (atk.sys.cloak_active) {
                                std::snprintf(msg_buf, sizeof(msg_buf),
                                    "%s cannot fire while cloaked!", atk.name.c_str());
                                msg_ttl = 120; push_log();
                                fire_mode = false; selected_weapon = -1;
                                break;  // exits the inner if block
                            }

                            bool is_seeking = (w.type == WeaponType::Drone ||
                                               w.type == WeaponType::PlasmaF ||
                                               w.type == WeaponType::PlasmaR ||
                                               w.type == WeaponType::PlasmaG);
                            bool in_arc = is_seeking ||
                                          rend.hex_in_arc(atk.position, atk.facing,
                                                           w.arc, tgt.position);
                            int  range  = atk.position.distance(tgt.position);
                            if (!atk.sensor_lock) range *= 2; // D6.123: no lock-on
                            bool range_ok = is_seeking || w.damage_at(range) > 0;
                            if (w.can_fire() && in_arc && range_ok) {
                                if (is_seeking) {
                                    // Launch as a seeking weapon entity
                                    if (try_launch_seeker(atk, selected_weapon, ships, hit, g_seekers, selected)) {
                                        w.fired = true;
                                        if (!w.is_instant()) { w.armed = false; w.charge = 0; }
                                        std::snprintf(msg_buf, sizeof(msg_buf),
                                            "%s launches %s â†’ %s",
                                            atk.name.c_str(), w.label.c_str(), tgt.name.c_str());
                                    }
                                } else if (w.type == WeaponType::ESG) {
                                    // ESG: area weapon — damages ALL enemies within radius simultaneously
                                    int hits = 0;
                                    w.fired = true;
                                    w.stored = 0;  // G23.0: discharge
                                    for (auto& enemy : ships) {
                                        if (enemy.destroyed || enemy.faction == atk.faction) continue;
                                        int er = atk.position.distance(enemy.position);
                                        int ed = w.damage_at(er);
                                        if (ed <= 0) continue;
                                        int sh = hit_facing(atk, enemy);
                                        enemy.apply_damage(ed, sh);
                                        ++hits;
                                    }
                                    std::snprintf(msg_buf, sizeof(msg_buf),
                                        "%s fires ESG (str %d) — %d ship(s) in radius",
                                        atk.name.c_str(), w.allocated, hits);
                                } else {
                                int fire_die = (rand() % 6) + 1;
                                int dmg   = w.roll_damage(range, fire_die);
                                // Nebula penalty: phasers lose 1 damage at range > 3 if either ship is in nebula
                                if (w.is_instant() && range > 3) {
                                    bool in_neb = (board.at(atk.position).terrain == Terrain::Nebula ||
                                                   board.at(tgt.position).terrain == Terrain::Nebula);
                                    if (in_neb) dmg = std::max(0, dmg - 1);
                                }
                                // P9 Dust Cloud: phaser effective range +5
                                if (w.is_instant()) {
                                    bool in_dust = (board.at(atk.position).terrain == Terrain::DustCloud ||
                                                    board.at(tgt.position).terrain == Terrain::DustCloud);
                                    if (in_dust) dmg = w.roll_damage(range + 5, fire_die);
                                }
                                int ecm_r = 0;
                                if (w.is_instant()) {
                                    int net = std::max(0, tgt.eaf.ecm - atk.eaf.eccm);
                                    // D13.0 Aegis fire control: ECM halved at close range (<=3), 1.5x at extreme range (>=9)
                                    if (range <= 3) net = net / 2;
                                    else if (range >= 9) net = net * 3 / 2;
                                    ecm_r = std::min(dmg, net);
                                    dmg   = std::max(0, dmg - ecm_r);
                                }
                                int shield = hit_facing(atk, tgt);
                                if (w.type == WeaponType::Hellbore)
                                    tgt.apply_damage_hellbore(dmg, shield);
                                else
                                    tgt.apply_damage(dmg, shield);
                                w.fired = true;
                                if (!w.is_instant()) { w.armed = false; w.charge = 0; }
                                if (dmg == 0 && (w.type == WeaponType::PhotonTorpedo ||
                                                  w.type == WeaponType::Phaser1 ||
                                                  w.type == WeaponType::Phaser2 ||
                                                  w.type == WeaponType::Disruptor)) {
                                    std::snprintf(msg_buf, sizeof(msg_buf),
                                        "%s fires %s -> %s: MISS (die %d, r%d)",
                                        atk.name.c_str(), w.label.c_str(),
                                        tgt.name.c_str(), fire_die, range);
                                    msg_ttl = 120; push_log();
                                } else

                                if (!tgt.last_dac_msg.empty()) {
                                    std::snprintf(msg_buf, sizeof(msg_buf),
                                        "INTERNAL HIT on %s: %s",
                                        tgt.name.c_str(), tgt.last_dac_msg.c_str());
                                } else {
                                    std::snprintf(msg_buf, sizeof(msg_buf),
                                        "%s fires %s: %d dmg to %s shield %d (r%d)%s%s",
                                        atk.name.c_str(), w.label.c_str(), dmg,
                                        tgt.name.c_str(), shield+1, range,
                                        ecm_r ? " [ECM reduced]" : "",
                                        w.type == WeaponType::Hellbore ? " [Hellbore bypass]" : "");
                                }
                                } // end else (non-seeking, non-ESG)
                                msg_ttl = 240; push_log();

                                if (tgt.destroyed && !tgt.exploded) {
                                    exp_ship = hit; exp_frame = 0;
                                    apply_explosion(tgt, ships, msg_buf, msg_ttl);
                                }
                                winner = check_victory(ships);
                                if (!winner.empty()) phase = GamePhase::GameOver;

                                selected_weapon = -1; fire_mode = false;
                            } else {
                                if (w.fired)
                                    std::snprintf(msg_buf, sizeof(msg_buf), "%s already fired this turn.", w.label.c_str());
                                else if (w.disabled)
                                    std::snprintf(msg_buf, sizeof(msg_buf), "%s is destroyed.", w.label.c_str());
                                else if (!w.is_ready())
                                    std::snprintf(msg_buf, sizeof(msg_buf), "%s not ready (%d/%d turns charged).", w.label.c_str(), w.charge, w.arming_turns);
                                else if (w.is_instant() && w.allocated == 0)
                                    std::snprintf(msg_buf, sizeof(msg_buf), "%s â€” allocate power in EAF first.", w.label.c_str());
                                else if (!in_arc)
                                    std::snprintf(msg_buf, sizeof(msg_buf), "%s â€” target not in firing arc.", w.label.c_str());
                                else
                                    std::snprintf(msg_buf, sizeof(msg_buf), "%s â€” out of range (r%d).", w.label.c_str(), range);
                                msg_ttl = 180; push_log();
                            }
                        }
                        else {
                            // FP1.5: phaser intercept of plasma torpedo in clicked hex
                            Ship& atk_i = ships[selected];
                            Weapon& wi  = atk_i.weapons[selected_weapon];
                            bool is_phas = (wi.type == WeaponType::Phaser1 ||
                                            wi.type == WeaponType::Phaser2 ||
                                            wi.type == WeaponType::Phaser3 ||
                                            wi.type == WeaponType::GatlingPhaser);
                            if (is_phas && wi.can_fire()) {
                                int sk_idx = -1;
                                for (int si = 0; si < (int)g_seekers.size(); ++si) {
                                    auto& sk = g_seekers[si];
                                    if (sk.pos == clicked &&
                                        (sk.type == WeaponType::PlasmaF ||
                                         sk.type == WeaponType::PlasmaR ||
                                         sk.type == WeaponType::PlasmaG) &&
                                        sk.owner_faction != atk_i.faction) {
                                        sk_idx = si; break;
                                    }
                                }
                                if (sk_idx >= 0) {
                                    auto& sk = g_seekers[sk_idx];
                                    int rng_i = atk_i.position.distance(clicked);
                                    if (!atk_i.sensor_lock) rng_i *= 2;
                                    int die_i = (rand() % 6) + 1;
                                    int dmg_i = wi.roll_damage(rng_i, die_i);
                                    int redux = dmg_i / 2;  // FP1.5: 2 phaser dmg = 1 warhead
                                    sk.damage = std::max(0, sk.damage - redux);
                                    wi.fired = true;
                                    if (sk.damage == 0) {
                                        g_seekers.erase(g_seekers.begin() + sk_idx);
                                        std::snprintf(msg_buf, sizeof(msg_buf),
                                            "%s fires %s at plasma: DESTROYED! (%d dmg)",
                                            atk_i.name.c_str(), wi.label.c_str(), dmg_i);
                                    } else {
                                        std::snprintf(msg_buf, sizeof(msg_buf),
                                            "%s fires %s at plasma: %d dmg -> warhead -%d (now %d)",
                                            atk_i.name.c_str(), wi.label.c_str(),
                                            dmg_i, redux, sk.damage);
                                    }
                                    msg_ttl = 180; push_log();
                                    selected_weapon = -1; fire_mode = false;
                                } else {
                                    std::snprintf(msg_buf, sizeof(msg_buf),
                                        "No plasma torpedo in that hex.");
                                    msg_ttl = 90;
                                }
                            }
                        }
                    } else if (hit >= 0) {
                        selected = (hit == selected) ? -1 : hit;
                        selected_weapon = -1; fire_mode = false;
                    } else if (selected >= 0 && !fire_mode) {
                        Ship& sel = ships[selected];
                        Hex fwd = forward_hex(sel);
                        if (!sel.destroyed && sel.move_available && clicked == fwd) {
                            Terrain mv_t = board.at(fwd).terrain;
                            if (mv_t == Terrain::Asteroid || mv_t == Terrain::Planet ||
                                mv_t == Terrain::TholianWeb || mv_t == Terrain::BlackHole) {
                                std::snprintf(msg_buf, sizeof(msg_buf), "Impassable -- turn first!");
                                msg_ttl = 90;
                            } else {
                                sel.position = fwd;
                                sel.move_available = false;
                                ++sel.moves_used;
                                ++sel.hexes_since_turn;
                            }
                        } else if (!sel.move_available) {
                            // Find next impulse this ship can move on
                            int next_imp = impulse + 1;
                            for (; next_imp <= 32; ++next_imp)
                                if (moves_this_impulse(sel.eaf.speed, next_imp)) break;
                            if (next_imp <= 32)
                                std::snprintf(msg_buf, sizeof(msg_buf),
                                    "%s moves on impulse %d (speed %d).",
                                    sel.name.c_str(), next_imp, sel.eaf.speed);
                            else
                                std::snprintf(msg_buf, sizeof(msg_buf),
                                    "%s: no more moves this turn (speed %d).",
                                    sel.name.c_str(), sel.eaf.speed);
                            msg_ttl = 90;
                        } else if (clicked != fwd) {
                            std::snprintf(msg_buf, sizeof(msg_buf),
                                "Ships move forward only. Q/E to turn.");
                            msg_ttl = 90;
                        }
                    }
                }
            }
        }

        // Explosion timer
        if (exp_ship >= 0 && ++exp_frame >= 30) exp_ship = -1;

        // â”€â”€ Render â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€
        SDL_GetWindowSize(win, &W, &H);
        int mx, my;
        SDL_GetMouseState(&mx, &my);

        SDL_SetRenderDrawColor(ren, 0, 0, 0, 255);
        SDL_RenderClear(ren);

        if (phase == GamePhase::Setup) {
            std::vector<std::string> names;
            std::vector<std::string> class_labels;
            std::vector<Faction>     factions;
            std::vector<bool>        inc;
            std::vector<ShipController> ctrls;
            for (auto& e : setup) {
                names.push_back(e.name);
                const auto& cl = faction_classes(e.faction);
                class_labels.push_back(cl[e.class_idx].second);
                factions.push_back(e.faction);
                inc.push_back(e.included);
                ctrls.push_back(e.controller);
            }
            rend.draw_setup_screen(
                names, class_labels, factions, inc, ctrls, W, H, setup_buttons,
                setup_dropdown_row, setup_dropdown_scroll,
                (setup_dropdown_row >= 0 && setup_dropdown_row < (int)setup.size())
                    ? &faction_classes(setup[setup_dropdown_row].faction) : nullptr);
            {
                char tbuf[64];
                std::snprintf(tbuf, sizeof(tbuf), "Turns: %d  ([ / ] to change)", max_turns);
                rend.draw_text(tbuf, 16, H - 24, LIGHTGRAY);
                rend.draw_text("S = save scenario   L = load scenario", 16, H - 44, GRAY);
            }
        } else {
            Hex hovered = rend.pixel_to_hex({(float)mx, (float)my});

            rend.draw_board(board);

            if (phase == GamePhase::TerrainSetup) {
                int bx = W - SIDEBAR_W + 10, tby = 10;
                rend.draw_text("TERRAIN SETUP", bx, tby, GOLD); tby += 30;
                rend.draw_text("Click hexes to paint terrain.", bx, tby, LIGHTGRAY); tby += 20;
                rend.draw_text("Keys 1-4 or click buttons. Enter/DONE when ready.", bx, tby, GRAY); tby += 20;
                const char* tnames[] = {
                    "1  Space (erase)","2  Nebula","3  Asteroid","4  Planet",
                    "5  Black Hole","6  Dust Cloud","7  Ion Storm",
                    "8  Gravity Rift (R=dir)","9  Tholian Web"};
                const Color tcols[]  = {
                    GRAY, Color{160,80,220,255}, Color{140,120,80,255}, Color{60,180,60,255},
                    Color{180,0,220,255}, Color{180,150,80,255}, Color{60,160,255,255},
                    Color{0,200,255,255}, Color{0,220,200,255}};
                for (int bi=0;bi<9;++bi) {
                    int tby2 = tby+bi*34;
                    Color tbc = (terrain_brush==bi) ? Color{60,120,200,255} : Color{30,30,50,220};
                    SDL_Rect tbr={bx, tby2, 240, 28};
                    SDL_SetRenderDrawColor(ren, tbc.r, tbc.g, tbc.b, tbc.a);
                    SDL_RenderFillRect(ren, &tbr);
                    Color tbe = (terrain_brush==bi) ? WHITE : GRAY;
                    SDL_SetRenderDrawColor(ren, tbe.r, tbe.g, tbe.b, tbe.a);
                    SDL_RenderDrawRect(ren, &tbr);
                    rend.draw_text(tnames[bi], bx+8, tby2+6, tcols[bi]);
                }
                if (terrain_brush == 7) {  // GravityRift: show current direction
                    static const char* dir_lbl[] = {"E","NE","NW","W","SW","SE"};
                    char rdl[32]; std::snprintf(rdl,sizeof(rdl),"  Rift dir: %s (R to cycle)",dir_lbl[rift_dir_brush]);
                    rend.draw_text(rdl, bx+8, tby+9*34+4, Color{0,200,255,255});
                }
                tby += 9*34+20;
                SDL_Rect dbr={bx, tby, 240, 32};
                SDL_SetRenderDrawColor(ren, 18, 85, 18, 255); SDL_RenderFillRect(ren, &dbr);
                SDL_SetRenderDrawColor(ren, 30, 180, 30, 255); SDL_RenderDrawRect(ren, &dbr);
                rend.draw_text("DONE  (Enter)", bx+8, tby+8, GREEN);
            }

            if (phase == GamePhase::Placement) {
                int bx = W - SIDEBAR_W + 10, pby = 10;
                rend.draw_text("SHIP PLACEMENT", bx, pby, GOLD); pby += 30;
                if (placement_idx < (int)ships.size()) {
                    char pbuf[128];
                    std::snprintf(pbuf, sizeof(pbuf), "Place: %s", ships[placement_idx].name.c_str());
                    rend.draw_text(pbuf, bx, pby, WHITE); pby += 20;
                    rend.draw_text("Click an empty hex to place it.", bx, pby, LIGHTGRAY); pby += 20;
                    int rem_p = 0;
                    for (int pi=placement_idx; pi<(int)ships.size(); ++pi)
                        if (ships[pi].controller!=ShipController::AI) ++rem_p;
                    std::snprintf(pbuf, sizeof(pbuf), "%d ship(s) remaining", rem_p);
                    rend.draw_text(pbuf, bx, pby, GRAY);
                }
                for (int pi=0;pi<(int)ships.size();++pi)
                    if (ships[pi].pos_confirmed) rend.draw_highlight(ships[pi].position, Color{0,120,180,120});
                if (placement_idx < (int)ships.size()) {
                    board.for_each([&](Hex ph, const Cell& pcell){
                        if (pcell.terrain==Terrain::Asteroid||pcell.terrain==Terrain::Planet) return;
                        bool occ=false;
                        for(auto& s:ships)if(s.pos_confirmed&&s.position.q==ph.q&&s.position.r==ph.r){occ=true;break;}
                        if (!occ) rend.draw_highlight(ph, Color{0,180,80,60});
                    });
                }
                rend.draw_ships(ships, -1);
            }

            if (phase == GamePhase::Impulse) {
                if (fire_mode && selected >= 0 && selected_weapon >= 0)
                    rend.draw_arc_overlay(ships[selected], selected_weapon, board, ships);
                else if (selected >= 0 && !fire_mode && !ships[selected].destroyed)
                    rend.draw_move_options(ships[selected], board, ships);
            }

            rend.draw_ships(ships, selected);

            // Draw seeking weapon markers
            {
                std::vector<SeekerView> sv;
                sv.reserve(g_seekers.size());
                for (auto& sk : g_seekers)
                    sv.push_back({sk.pos, (sk.type == WeaponType::Drone) ? 0 : 1});
                rend.draw_seekers(sv);
            }

            if (exp_ship >= 0)
                rend.draw_explosion(ships[exp_ship].position, exp_frame);

            if (board.contains(hovered) && mx < board_area_w && phase == GamePhase::Impulse)
                rend.draw_highlight(hovered, YELLOW);

            rend.draw_hud(turn, impulse, max_turns, W, ships, llm_on);

            // Selected-ship turn-mode info bar
            if (selected >= 0 && selected < (int)ships.size()) {
                const Ship& ss = ships[selected];
                int tm = turn_mode(ss.eaf.speed, ss.turn_mode_cat);
                char sinfo[128];
                if (tm == 0) {
                    std::snprintf(sinfo, sizeof(sinfo),
                        "%s  |  Spd %d  |  TM: free (stationary -- set speed in EAF)  |  [F1]=SSD",
                        ss.name.c_str(), ss.eaf.speed);
                } else {
                    int need = tm - ss.hexes_since_turn;
                    const char* can = (need <= 0) ? "CAN TURN" : nullptr;
                    std::snprintf(sinfo, sizeof(sinfo),
                        "%s  |  Spd %d  |  TM-%c (%d hex/turn)  |  %s  |  [F1]=SSD",
                        ss.name.c_str(), ss.eaf.speed, 'A' + tm - 1, tm,
                        can ? can : (std::to_string(need) + " more hex(es) before turn").c_str());
                }
                rend.draw_text(sinfo, 8, 32, Color{180, 210, 255, 255});
            }

            // Pause menu overlay (drawn last, on top of everything)
            if (pause_menu_open)
                rend.draw_pause_menu(W, H, pause_buttons);

            Ship* sel_ship = (selected >= 0) ? &ships[selected] : nullptr;
            if (show_ssd) {
                rend.draw_ssd_panel(sel_ship, W, H);
            } else {
                rend.draw_sidebar(sel_ship, selected_weapon, W, H, weapon_buttons);
            }

            if (phase == GamePhase::Planning && eaf_player < (int)ships.size()) {
                if (!eaf_hidden)
                    rend.draw_eaf_modal(ships[eaf_player], turn, W, H, eaf_buttons, eaf_scroll);
                else
                    rend.draw_text("[TAB] EAF panel hidden - press TAB to show", 10, 35, GOLD);
                char whose[80];
                std::snprintf(whose, sizeof(whose), "Allocating: %s",
                              ships[eaf_player].name.c_str());
                rend.draw_text(whose, 10, 35, GOLD);
                // Show crew advice overlay for player ships
                if (ships[eaf_player].controller != ShipController::AI &&
                    eaf_player < (int)crew_advices.size()) {
                    crew_row_rects.clear();
                    rend.draw_crew_advice(crew_advices[eaf_player], W, H,
                                          crew_edit_role, &crew_row_rects);
                }
            }

            if (phase == GamePhase::GameOver)
                rend.draw_game_over(winner, W, H);

            if (phase == GamePhase::Impulse)
                rend.draw_combat_log(combat_log, W, H);

            if (msg_ttl > 0) {
                rend.draw_text(msg_buf, 10, H - 30, ORANGE);
                --msg_ttl;
            }
        }

        SDL_RenderPresent(ren);
        SDL_Delay(16);
    }

    if (font) TTF_CloseFont(font);
    SDL_DestroyRenderer(ren);
    SDL_DestroyWindow(win);
    TTF_Quit();
    SDL_Quit();
    return 0;
}





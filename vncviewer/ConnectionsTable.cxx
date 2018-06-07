#include "ConnectionsTable.h"
#include "ServerDialog.h"

#include <FL/Fl_Light_Button.H>
#include <FL/Fl_Check_Button.H>
#include <FL/Fl_Button.H>
#include <FL/Fl_Output.H>
#include <FL/fl_draw.H>
#include <algorithm>

static const char* TABLE_HEADER[] = { "VNC Server", "Pin", "Run" };

ConnectionsTable::ConnectionsTable(int x, int y, int w, int h, HostnameList &history)
: Fl_Table(x,y,w,h)
, history(history)
, callbackServername("")
, callbackPinned(false) {
    col_header(1);
    col_resize(0);
    row_header(0);
    row_resize(0);
    when(FL_WHEN_RELEASE);
    end();
    setRecentConnections();
}

ConnectionsTable::~ConnectionsTable() {
}

void ConnectionsTable::draw_cell(TableContext context,
    int R, int C, int X, int Y, int W, int H) {
    switch (context) {
        case CONTEXT_STARTPAGE:
            fl_font(FL_HELVETICA, FL_NORMAL_SIZE);
        break;

        case CONTEXT_RC_RESIZE: {
            int X,Y,W,H;
            int index = 0;
            for (int r=0;r<rows();++r) {
                for (int c=0;c<cols();++c) {
                    if(index>=children())break;
                    find_cell(CONTEXT_TABLE,r,c,X,Y,W,H);
                    child(index++)->resize(X,Y,W,H);
                }
            }
            init_sizes();
        }
        break;

        case CONTEXT_ROW_HEADER:
        break;

        case CONTEXT_COL_HEADER: {
            fl_push_clip(X,Y,W,H);
            fl_draw_box(FL_THIN_UP_BOX,X,Y,W,H,col_header_color());
            fl_color(FL_BLACK);
            fl_draw(TABLE_HEADER[C],X,Y,W,H,FL_ALIGN_CENTER);
            fl_pop_clip();
        }
        break;

        case CONTEXT_CELL:
        break;

        default:
        return;
    }
}

void ConnectionsTable::setRecentConnections() {
    // Sort, if both are pinned base it on rank. Otherwise if it's pinned it must be first
    std::sort(history.begin(),history.end(), [](RankedHostName const &t1, RankedHostName const &t2){
        if (std::get<HostTupleIndex::PINNED>(t1) == std::get<HostTupleIndex::PINNED>(t2))
            return std::get<HostTupleIndex::RANK>(t1) < std::get<HostTupleIndex::RANK>(t2);
        else if (std::get<HostTupleIndex::PINNED>(t1))
            return true;
        return false;
    });

    clear();
    cols(NUM_COLS);
    rows(history.size());

    begin(); {
        col_width(SERVER_COL,tiw-PIN_COL_SIZE-RUN_COL_SIZE);
        col_width(PIN_COL,PIN_COL_SIZE);
        col_width(RUN_COL,RUN_COL_SIZE);
        for ( size_t r = 0; r < history.size(); ++r) {
            int X,Y,W,H;

            find_cell(CONTEXT_TABLE,r,SERVER_COL,X,Y,W,H);
            Fl_Output *out = new Fl_Output(X,Y,W,H);
            out->value(strdup(std::get<HostTupleIndex::NAME>(history[r]).c_str()));

            find_cell(CONTEXT_TABLE,r,PIN_COL,X,Y,W,H);
            Fl_Check_Button *pin = new Fl_Check_Button(X,Y,W,H);
            pin->value(std::get<HostTupleIndex::PINNED>(history[r]));

            find_cell(CONTEXT_TABLE,r,RUN_COL,X,Y,W,H);
            Fl_Button *run = new Fl_Button(X,Y,W,H,"@>");
            run->callback(handleRun, static_cast<void*>(this));
        }
    }
    end();
}

void ConnectionsTable::handleRun(Fl_Widget *widget, void* data) {
    ConnectionsTable *conTable = reinterpret_cast<ConnectionsTable*>(data);
    conTable->handleRun(widget);
}

void ConnectionsTable::handleRun(Fl_Widget *widget) {
    int row = find(widget) / NUM_COLS;

    Fl_Output *out = reinterpret_cast<Fl_Output*>(child(NUM_COLS*row + SERVER_COL));
    Fl_Check_Button *check = reinterpret_cast<Fl_Check_Button*>(child(NUM_COLS*row + PIN_COL));

    callbackServername =  out->value();
    callbackPinned = check->value();

    updatePinnedStatus();

    do_callback(CONTEXT_CELL,row,RUN_COL);
}

void ConnectionsTable::updatePinnedStatus(std::string newServername) {
    history.clear();
    int total_children = children();

    // First append the new server
    if (!newServername.empty()) {
        RankedHostName host = std::make_tuple(0,newServername,false);
        history.push_back(std::move(host));
    }

    // Then rearrange with correct order in vector
    auto unpinned_it = history.begin();
    int row;
    for (row = 0; row < total_children / NUM_COLS; ++row) {
        Fl_Output *out = reinterpret_cast<Fl_Output*>(this->child(NUM_COLS * row + SERVER_COL));
        Fl_Check_Button *check = reinterpret_cast<Fl_Check_Button*>(this->child(NUM_COLS * row + PIN_COL));
        RankedHostName host = std::make_tuple(row,out->value(),check->value());

        if (check->value()) {
            unpinned_it = history.insert(unpinned_it,std::move(host))+1;
        } else {
            history.push_back(std::move(host));
        }
    }

    if (history.size() > MAX_ROWS) { //TODO: Add parameter for MAX_ROWS
        history.pop_back();
    }

    auto last = std::unique(history.begin(), history.end(), [](RankedHostName const &t1, RankedHostName const &t2){
        return std::get<HostTupleIndex::NAME>(t1) == std::get<HostTupleIndex::NAME>(t2);
    });
    history.erase(last,history.end());

    // Setting up the right rank number
    row = 0;
    for (auto it = history.begin(); it != history.end(); ++it, ++row) {
        std::get<HostTupleIndex::RANK>(*it) = row;
    }
}

std::string ConnectionsTable::callback_servername() {
    return callbackServername;
}

bool ConnectionsTable::callback_pinned() {
    return callbackPinned;
}

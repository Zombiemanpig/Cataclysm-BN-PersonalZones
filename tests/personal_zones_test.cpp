#include "avatar.h"
#include "cata_utility.h"
#include "catch/catch.hpp"
#include "clzones.h"
#include "map_helpers.h"
#include "player_helpers.h"
#include "state_helpers.h"
#include "type_id.h"

#include <algorithm>

TEST_CASE( "personal loot zones follow the avatar until a sort pins them", "[zones][personal]" )
{
    clear_all_state();

    avatar &you = get_avatar();
    auto &mgr = zone_manager::get_manager();
    const auto cleanup = on_out_of_scope( [&mgr]() {
        mgr.clear_sort_filter();
        for( ;; ) {
            auto listed = mgr.get_zones();
            const auto found = std::ranges::find_if( listed, []( const auto &ref ) {
                return ref.get().get_is_personal();
            } );
            if( found == listed.end() ) {
                break;
            }
            mgr.remove( found->get() );
        }
    } );
    const auto origin = tripoint_bub_ms{ 60, 60, 0 };
    you.setpos( origin );

    const auto start = tripoint_rel_ms{ -1, -1, 0 };
    const auto end = tripoint_rel_ms{ 1, 0, 0 };
    mgr.add( "personal unsorted", zone_type_id( "LOOT_UNSORTED" ), faction_id( "your_followers" ),
             false, true, start, end );

    const auto placed_at = you.abs_pos();
    const auto *zone = mgr.get_zone_at( placed_at + start, zone_type_id( "LOOT_UNSORTED" ) );
    REQUIRE( zone != nullptr );
    CHECK( zone->get_is_personal() );
    CHECK( zone->get_start_point() == placed_at + start );
    CHECK( zone->get_end_point() == placed_at + end );

    you.setpos( origin + tripoint_rel_ms{ 4, 0, 0 } );
    const auto moved_to = you.abs_pos();
    CHECK( mgr.get_zone_at( moved_to + start, zone_type_id( "LOOT_UNSORTED" ) ) != nullptr );
    CHECK( mgr.get_zone_at( placed_at + start, zone_type_id( "LOOT_UNSORTED" ) ) == nullptr );

    mgr.apply_sort_filter( loot_sort_selection::personal_only, moved_to );
    you.setpos( origin + tripoint_rel_ms{ 8, 0, 0 } );
    CHECK( mgr.get_zone_at( moved_to + start, zone_type_id( "LOOT_UNSORTED" ) ) != nullptr );
    CHECK( mgr.get_zone_at( you.abs_pos() + start, zone_type_id( "LOOT_UNSORTED" ) ) == nullptr );

    const auto player_points = mgr.get_point_set_loot( moved_to, 5, false );
    const auto npc_points = mgr.get_point_set_loot( moved_to, 5, true );
    CHECK_FALSE( player_points.empty() );
    CHECK( npc_points.empty() );

    mgr.clear_sort_filter();
    CHECK( mgr.get_zone_at( you.abs_pos() + start, zone_type_id( "LOOT_UNSORTED" ) ) != nullptr );
    CHECK_FALSE( mgr.personal_zones_are_pinned() );

    mgr.apply_sort_filter( loot_sort_selection::regular_only, you.abs_pos() );
    CHECK( mgr.get_near( zone_type_id( "LOOT_UNSORTED" ), you.abs_pos(), 5 ).empty() );
    mgr.clear_sort_filter();
    CHECK_FALSE( mgr.get_near( zone_type_id( "LOOT_UNSORTED" ), you.abs_pos(), 5 ).empty() );
}

l4_5_protocol = Proto("L4.5", "Layer 4.5 Protocol")

-- Header fields
proto_flag = ProtoField.uint32 ("l4_5_protocol.proto_flag", "protoFlag", base.HEX)
app_id     = ProtoField.string("l4_5_protocol.app_id"    , "Application ID" , base.ASCII)

l4_5_protocol.fields = { proto_flag, app_id }

local function heuristic_checker(buffer, pinfo, tree)
    -- guard for length
    length = buffer:len()
    -- print("Hello heuristic entry")
    -- we use a 32 byte tag prior to DNS
    if length < 32 then return false end
    -- flag = XTAG -> 0x58 54 41 47
    -- buffer(offset, length)
    local potential_proto_flag = buffer(0,4):uint()
    if potential_proto_flag ~= 0x58544147 then return false end

    -- app tag is 16 bytes
    -- local potential_app_id = buffer(4,16):
    --
    -- -- 12 byte trailer = XXXXXXXXXXXX
    -- local potential_proto_flag_trailer = buffer(20,4):uint()
    -- if potential_proto_flag_trailer ~= 0x58585858 then return false end

    -- print("Hello heur exit")

    l4_5_protocol.dissector(buffer, pinfo, tree)
    return true
end

function l4_5_protocol.dissector(buffer, pinfo, tree)
    -- print("Hello dissector entry")
    length = buffer:len()
    if length == 0 then return end

    local udp_next = Dissector.get("dns")

    local potential_proto_flag = buffer(0,4):uint()
    if potential_proto_flag ~= 0x58544147 then
      udp_next(buffer(0, length):tvb(), pinfo, tree)
      return
    end

    pinfo.cols.protocol = l4_5_protocol.name
    local subtree = tree:add(l4_5_protocol, buffer(), "Layer4.5 Cust Protocol Data")
    -- Header
    subtree:add(proto_flag, buffer(0,4))
    subtree:add(app_id, buffer(4,16))

    local udp_next = Dissector.get("dns")
    -- print(table.concat(udp_next,", "))
    udp_next(buffer(20, length - 20):tvb(), pinfo, tree)
end

DissectorTable.get("udp.port"):add(53, l4_5_protocol)
l4_5_protocol:register_heuristic("udp", heuristic_checker)

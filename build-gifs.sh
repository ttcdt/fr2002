#!/bin/sh

ids="cobra t-rex t-rex-4arm snakeman ghoul dhole gorgon head dwarf plant priest ant-floor ant-ceiling cyclops two-head tree-floor fountain clock pyre chain phoetus egg shark-underwater snake shark-surface alien sentinel shadow robot-blue robot-red dust-monster robot-car mallok robotina t-rex robert mutant tree-swamp claw mantis golden-ghoul tombstone zombie cultist the-thing apprentice"
brk="tree-floor fountain clock pyre chain phoetus egg tree-swamp tombstone"
num=0
delay=20
delay_2=20
delay_b=50
delay_d=200

for id in ${ids} ; do
    gif="freaks-${id}.gif"
    echo "$gif"

    walk_0=$(printf "demons_%03d.tga" $num)
    num=$(($num + 1))
    walk_1=$(printf "demons_%03d.tga" $num)
    num=$(($num + 1))
    attack_0=$(printf "demons_%03d.tga" $num)
    num=$(($num + 1))
    attack_1=$(printf "demons_%03d.tga" $num)
    num=$(($num + 1))
    bleed=$(printf "demons_%03d.tga" $num)
    num=$(($num + 1))
    corpse=$(printf "demons_%03d.tga" $num)
    num=$(($num + 1))

    if echo $brk | grep -q $id ; then
        convert -dispose Background \
        -delay ${delay_2} \
        ${walk_0} ${walk_1} \
        ${walk_0} ${walk_1} \
        ${walk_0} ${walk_1} \
        ${walk_0} ${walk_1} \
        -delay ${delay_b} ${bleed} \
        -delay ${delay_d} ${corpse} \
        ${gif}
    else
        convert -dispose Background \
        -delay ${delay} \
        ${walk_0} ${walk_1} \
        ${walk_0} ${walk_1} \
        ${walk_0} ${walk_1} \
        ${walk_0} ${walk_1} \
        ${attack_0} ${attack_1} \
        ${attack_0} ${attack_1} \
        ${attack_0} ${attack_1} \
        ${walk_0} ${walk_1} \
        ${walk_0} ${walk_1} \
        ${walk_0} ${walk_1} \
        ${walk_0} ${walk_1} \
        ${attack_0} ${attack_1} \
        ${attack_0} ${attack_1} \
        ${walk_0} ${walk_1} \
        ${walk_0} ${walk_1} \
        -delay ${delay_b} ${bleed} \
        -delay ${delay_2} \
        ${walk_0} ${walk_1} \
        ${walk_0} ${walk_1} \
        -delay ${delay} \
        ${attack_0} ${attack_1} \
        ${attack_0} ${attack_1} \
        ${walk_0} ${walk_1} \
        ${walk_0} ${walk_1} \
        -delay ${delay_b} ${bleed} \
        -delay ${delay_d} ${corpse} \
        ${gif}
    fi

    delay_2=$(($delay_2 + 1))
done

ids="feet brain hand guts torso skull eyes heart bones baby"
delay=20
num=0

for id in ${ids} ; do
    gif="freaks-${id}.gif"
    echo "$gif"

    anim_0=$(printf "items_e1_%03d.tga" $num)
    num=$(($num + 1))
    anim_1=$(printf "items_e1_%03d.tga" $num)
    num=$(($num + 1))

    convert -dispose Background \
        -delay ${delay_2} \
        ${anim_0} ${anim_1} \
        $gif

    delay_2=$(($delay_2 + 1))
done

ids="green-card blue-card red-card golden-card grey-card green-ball blue-ball red-ball golden-ball grey-ball"
delay=20
num=0

for id in ${ids} ; do
    gif="freaks-${id}.gif"
    echo "$gif"

    anim_0=$(printf "items_e2_%03d.tga" $num)
    num=$(($num + 1))
    anim_1=$(printf "items_e2_%03d.tga" $num)
    num=$(($num + 1))

    convert -dispose Background \
        -delay ${delay_2} \
        ${anim_0} ${anim_1} \
        $gif

    delay_2=$(($delay_2 + 1))
done

ids="idol-1 idol-2 idol-3 cross sceptre crown medal belt right-boot left-boot"
delay=20
num=0

for id in ${ids} ; do
    gif="freaks-${id}.gif"
    echo "$gif"

    anim_0=$(printf "items_e3_%03d.tga" $num)
    num=$(($num + 1))
    anim_1=$(printf "items_e3_%03d.tga" $num)
    num=$(($num + 1))

    convert -dispose Background \
        -delay ${delay_2} \
        ${anim_0} ${anim_1} \
        $gif

    delay_2=$(($delay_2 + 1))
done

for f in $(find -name "*mappings"); do
    cp $f $f.py
    echo $f
    cloc $f.py | tail -n 2 | head -n 1 | tr -s ' ' | cut -d' ' -f5
done

cmappers=("../examples/orimapper/circuit_mapper.cc" "../examples/orimapper/stencil_mapper.cc" "../examples/orimapper/pennant_mapper.cc")
for f in ${cmappers[@]}; do
    echo $f
    cloc $f | tail -n 2 | head -n 1 | tr -s ' ' | cut -d' ' -f5
done

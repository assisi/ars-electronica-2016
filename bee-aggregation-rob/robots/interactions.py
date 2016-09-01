#{{{ notes
# using the nbg file to determine the interaction strengths.

# I don't care what the names are so much, but I do want to know what the
# weights are. Annoyingly I think we need to manually specify every link, but
# perhaps these can simply be Link_1_2, Link_X_Y, ...
# since the RTC file needs a name to map it to.

'''
To SEND a message
- we must specify the body text, and destination CASU
-> we must know who our out-neighbours are, by name. No other info
   is needed here


To RECV a message
- the message has
   - a body (text, split and convert to a float)
   - a source/sender
- the present xinhib code reads the label to determine whether this
  is friend or enemy
- but we are able to lookup the weight of a given edge based on the
  sender (src_phys)


'''

'''
how to handle layers?

a. flatten the graph, by inserting nodes (with a failsafe) to a new graph
   for every parsed node, with SG dropped; same for edges.

b. for each node, go through all the sub-graphs to look for a match
   each time we read anything

- ultimately, what we want to produce is a map (dict) for a given, named
  casu, which provides all of the sources and destinations, weights and labels.

- so which is easier?
    - going to traverse the graph just once with method A



'''
#}}}


# This file will allow us to
import pygraphviz as pgv
import os



#{{{ funcs

def get_outmap(fg, casu, verb=False):
    '''
    given the flattened graph, `fg`, find a map of connections for
    which the node `casu` should emit messages to.

    return a dict whose keys are the destinations and whose values
    are edge labels

    '''
    if casu not in fg:
        raise KeyError

    sendto = {}

    for src, _dest in fg.out_edges(casu):
        # attributes does not have a .get(default=X) so make useable copy
        attr = dict(fg.get_edge(src, _dest).attr)
        lbl = attr.get('label')
        # trim any layer info from the edge src/dest
        dest = str(_dest).split('/')[-1]

        if verb: print "\t--> {} label: {}".format(dest, lbl)
        sendto[dest] = lbl

    return sendto

def get_inmap(fg, casu, default_weight=None, verb=False):
    '''
    given the flattened graph, `fg`, find a map of connections for
    which the node `casu` may receive messages from

    return a dict whose keys are the sources and whose values are
    dicts with entries for labels and edgeweight.

    the value of `default_weight` is set on all weights if one is not
    specified in the graph file


    '''
    if casu not in fg:
        raise KeyError

    recvfrom = {}
    # and a map of incoming weights
    if verb: print "expect to receive from..."
    for src, dest in fg.in_edges(casu):
        attr = dict(fg.get_edge(src, dest).attr)
        lbl = attr.get('label')
        w = float(attr.get('weight', default_weight))

        if verb: print "\t<-- {} w={:.2f} label: {}".format(src, w, lbl)
        recvfrom[src] = { 'w': w, 'label': lbl }

    return recvfrom




def show_inout(nbg):
    for casu in nbg.nodes():
        print "\n*** {} ***".format(casu)

        # need to know who will send to
        # (why? how do the messages work?)
        my_sendto = []
        my_recvfrom = []
        print "Send to..."
        for src, _dest in nbg.out_edges(casu):
            # attributes set is like a dict but not completely
            # - without a mechansim for get+default
            # so make a copy that is useable.
            attr = dict(nbg.get_edge(src, _dest).attr)
            lbl = attr.get('label')
            # the edge might be multi-layer, so throw away the layer since
            # not needed here
            dest = str(_dest).split('/')[-1]

            print "\t--> {} label: {}".format(dest, lbl)
            my_sendto.append(dest)

        # and a map of incoming weights
        print "expect to receive from..."
        for src, dest in nbg.in_edges(casu):
            attr = dict(nbg.get_edge(src, dest).attr)
            lbl = attr.get('label')
            w = float(attr.get('weight', 0.0))

            print "\t<-- {} w={:.2f} label: {}".format(src, w, lbl)
            my_recvfrom.append(src)


def flatten_AGraph(nbg):
    '''
    process a multi-layer CASU interaction graph file and return
    a flattened graph.

    Note: does not support node properties!
    '''
    g = pgv.AGraph(directed=True, strict=False) # allow self-loops.

    for _n in nbg.nodes():
        # trim any layer info off the node
        n = _n.split('/')[-1]
        #print n, _n
        g.add_node(n)


    for i, (_src, _dest) in enumerate(nbg.edges()):
        # trimmed versions
        s = _src.split('/')[-1]
        d = _dest.split('/')[-1]

        # if it doesn't exist already, add edge
        if not (s in g and d in g):
            print "[W] s,d not both present. not adding edge"
            continue
        #else:
        #    print "[I] adding {} -> {} ({} : {})".format(s, d,
        #                                                 s.split('-')[-1],
        #                                                 d.split('-')[-1])

        g.add_edge(s, d)
        #print "now we have {} edges (pre: {}, delta {})\n".format(
        #    pre, post, post-pre)

        # add any attributes to the flattened graph
        attr = dict(nbg.get_edge(_src, _dest).attr)

        e = g.get_edge(s, d)
        for k, v in attr.iteritems():
            e.attr[k]= v

    return g

#}}}


if __name__ == "__main__":
    f_nbg = "virt_virt_98_32.nbg" # harder to handled due to mixed layers
    #f_nbg = "virt_sep_dual_12_34.nbg"

    nbg = pgv.AGraph(os.path.join("../", "dep_sim", f_nbg))

    g2 = flatten_AGraph(nbg)

    casu = g2.nodes()[0]

    imap = get_inmap(g2, casu)
    omap = get_outmap(g2, casu)

    print casu, imap, omap

    if 1:
        print "=" * 60
        show_inout(nbg)
        print "\n\n"
        print "=" * 60
        show_inout(g2)



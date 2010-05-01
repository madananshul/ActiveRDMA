package dfs;

import examples.dht.DHT;

public class DFSIter
{
    DHT m_dht;
    int m_bins;

    int _bin, _ptr;

    public DFSIter(DFS dfs, int bin, int ptr)
    {
        m_dht = dfs.dht();
        m_bins = m_dht.getNBins();

        _bin = bin;
        _ptr = ptr;
    }

    public int bin()
    {
        return _bin;
    }

    public int ptr()
    {
        return _ptr;
    }

    public void next()
    {
        if (done()) return;

        if (_ptr != 0)
            _ptr = m_dht.getNext(_ptr);

        while (_ptr == 0)
        { 
            _bin++;
            if (_bin == m_bins)
            {
                _bin = EOF_bin;
                break;
            }

            _ptr = m_dht.getHead(_bin);
        }
    }

    public boolean done()
    {
        return _bin == EOF_bin;
    }

    public void findNext(String pattern)
    {
        while (!done())
        {
            if (key().matches(pattern)) break;
            next();
        }
    }

    public String key()
    {
        if (done()) return null;

        return m_dht.getKey(_ptr);
    }

    public final static int EOF_bin = -1;
}
